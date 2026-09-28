# COLA Julia: project guide

## Purpose

Expose COLA's native event types to Julia and load pure-Julia physics filters
into C++ calculations. Users write normal Julia packages; they should not need
to understand C++ interop. Keep the implementation small and avoid speculative
features, extra configuration and bulk-operation APIs. Windows is out of scope.

## Layout

- `Project.toml`, `src/`, `test/`: the Julia package and its native Julia tests.
- `src/COLA.jl`: public types, field accessors, keyword parameter schemas and
  the installed binding-library loader.
- `src/bridge.jl`: private type resolution, filter construction, dispatch and
  exception capture. No registration table or object-ID registry.
- `COLA_Julia/`: C++ interfaces, runtime and module entry point. Headers and
  implementations stay together; the namespace is `cola::jl`.
- `bindings/`: WrapIt configuration, generation target and manual accessors.
- `deps/build.jl`: Pkg entry point for native build and installation.
- `cmake/`: installed package configuration template.
- Examples live only on the `examples` branch; do not merge them into `main`.
- `tests/fixtures/JuliaTestFilters/`: Julia fixtures for integration tests.
- `tests/`: C++ integration tests; `tests/consumer/` is a separate CMake project
  using only installed native targets/libraries.

## Build and installation contract

- `Pkg.build("COLA")` is the supported installation entry point. CMake builds
  native code; it does not copy or install Julia source packages.
- Install into the prefix exported as `COLA_DIR` by `COLAConfig.cmake`.
  Use standard `find_package(COLA CONFIG REQUIRED)` discovery without custom
  environment hints. Do not auto-install COLA or use sudo.
- The build script supplies the running Julia executable and its environment's
  `JlCxx_DIR`. Do not restore CMake-side Julia package discovery or installation.
- Install the adapter in `lib/COLA_Julia`, bindings in its `bindings/`
  subdirectory, headers in `include/COLA_Julia`, and exported CMake configuration
  in `lib/cmake/COLA_Julia`. Keep bindings out of the module directory's immediate
  library list: COLA's loader expects the plugin entry point there.
- Preserve `find_package(COLA_Julia CONFIG REQUIRED)` and the target
  `COLA_Julia::COLA_Julia`, as well as runtime loading through `cola::LoadModule`.
- Generated `deps/deps.jl` records the prefix only after successful installation.
  `COLA.prefix_path()` exposes it. Generated files and build outputs are not source.
- Installation overwrites shared native files outside Pkg ownership. Building
  and running must resolve the same native CxxWrap artifact; version numbers
  alone do not establish that. Do not silently alter the user's global environment.

## Interop invariants

- One embedded runtime, used and finalized on its initializing thread. No worker
  queue, per-filter project selection or custom sysimage support.
- C++ owns filter handles through RAII and persistent CxxWrap GC roots. Destruction
  calls `close!` and releases the root even if closing fails.
- Events passed to Julia are borrowed. Field accessors must preserve references,
  not copy entire events. Vector mutations can invalidate particle references.
- Preserve particle bounds checks, exception propagation and `invokelatest`
  where needed for newly loaded package methods. Custom parser errors propagate.
- Resolve dotted package/type names without evaluating user-provided expressions.
  Schema parameters become constructor keywords; no user registry or kind field.
- Manual accessors include generated C++ in the same translation unit to keep
  CxxWrap type traits consistent. Do not split compilation casually.
- C++20 is required by the currently generated WrapIt template lambdas.

## Verification

Use the README's installation and test commands. Run `Pkg.test("COLA")` and the
fixture package's Julia tests. For native packaging changes, configure `tests/consumer`
against the installed prefix, then run both its linked and plugin tests with
`JULIA_PROJECT` set to an environment containing COLA and JuliaTestFilters.
Test significant packaging changes from a temporary copy outside the checkout;
do not substitute build-tree libraries for installed-library verification.

Use the repository's clang-format, clang-tidy and cmake-format configurations.
Separate methods with blank lines. Preserve user changes and do not weaken tests
just to make them pass. Apple Silicon macOS is validated; do not claim Linux
support has been tested without running it.
