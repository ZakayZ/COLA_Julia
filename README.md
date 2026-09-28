# COLA Julia

COLA generators, converters and writers are ordinary Julia packages. CxxWrap
exposes COLA's real C++ event types; callbacks modify them in place. The same
package runs in Julia's REPL, in `Pkg.test()`, and in an embedded COLA calculation.
The build targets macOS and Linux; this rewrite
has been validated on Apple Silicon macOS. Linux has not yet been validated.

## Build with Julia's package manager

Install COLA, Julia, CMake and a C++20 compiler first. The repository root is
the Julia package; it includes the native sources needed for a local build.

```julia
using Pkg
ENV["COLA_DIR"] = "/path/to/cola/install/lib/cmake/COLA"
Pkg.develop(path="/path/to/COLA_Julia")
Pkg.build("COLA")
Pkg.test("COLA")
```

Pkg resolves the declared CxxWrap and WrapIt dependencies. `deps/build.jl`
uses that same environment and Julia executable to compile both libraries.
It does not install system tools or modify the existing COLA installation.
Build files go to `deps/build`, bindings to `deps/libCOLA_JuliaBindings`,
with the platform library extension, and the adapter to `deps/lib/COLA_Julia`.
Pass the package's `deps/lib` directory to COLA's module loader. It can be
located with `normpath(joinpath(dirname(pathof(COLA)), "..", "deps", "lib"))`.
Use the same Julia dependency environment for the calculation, and rebuild
after changing native dependencies. `COLA_DIR` is optional if CMake can already
find the installed COLA package.

## Build directly with CMake

Install Julia (including libjulia), COLA, CMake and a C++20 compiler.
CxxWrap and WrapIt must already be installed in the selected Julia environment.
CMake uses that environment (the default Julia environment, or `JULIA_PROJECT`)
without installing or updating packages. Missing packages fail configuration.
Use the same environment when configuring and building.

From this directory:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/cola/install
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/cola/install
```

The build produces two independent libraries:

- `libCOLA_JuliaBindings`: data bindings, loaded by Julia.
- `libCOLA_Julia`: the COLA module and embedded runtime, loaded by COLA.

The ordinary `COLA` Julia package is staged at `build/julia/COLA` and installed
at `share/COLA_Julia/julia/COLA`, with the bindings in its `deps` directory.
The build toolchain and calculation must resolve the same CxxWrap native artifact.
The package pins its supported native CxxWrap release. Rebuild after changing
native dependencies; an artifact must be compatible with the host COLA C++ ABI.
Prebuilt COLA JLL artifacts are not published by this project yet.

C++ sources and headers remain together in `COLA_Julia/`, with namespace `cola::jl`.
`bindings/CMakeLists.txt` generates the CxxWrap bindings with WrapIt into
`build/bindings/generated/` and builds their library. Generated files are not
stored in source control. WrapIt is a build dependency, not a runtime dependency
for user physics packages.

## Develop and distribute a package

Use a standard Julia package with `Project.toml`, `src/MyPhysics.jl`, and
`test/runtests.jl`. Add the built or installed API with `Pkg.develop`:

```julia
using Pkg
Pkg.activate("/path/to/MyPhysics")
Pkg.develop(path="/path/to/COLA_Julia/build/julia/COLA")
```

User packages contain only Julia. Share them through Git or a Julia registry.
Until COLA itself is published in a registry, recipients also develop its installed
package. One shared native adapter serves all user packages.

Example `src/MyPhysics.jl`:

```julia
module MyPhysics
using COLA
import COLA: convert!

struct MomentumScaler <: Converter
    scale::Float64
end
MomentumScaler(; scale::Real=1.0) = MomentumScaler(Float64(scale))

function convert!(filter::MomentumScaler, event)
    for particle in particles(event)
        vector = momentum(particle)
        set_energy!(vector, energy(vector) * filter.scale)
    end
    return nothing
end

COLA.parameters(::Type{MomentumScaler}) = (
    scale=parameter(Float64; default=1.0),
)
end
```

Generators subtype `Generator` and implement `generate!(filter, event)`, filling an
empty event supplied by COLA. Converters implement `convert!(filter, event)`.
Writers implement `write!(filter, event)`. These are in-place callbacks; their return
value does not replace the native event. All may implement `COLA.close!(filter)`
for resource cleanup. Close exceptions are reported to stderr during native teardown.

Define `COLA.parameters(::Type{YourFilter})` to return a named tuple of parameter
schemas. The default schema is empty for filters without configuration. No registry
or alias is needed: the filter kind is determined by its abstract supertype.
A parameter without a default is required. XML strings are parsed using the declared
type before calling the constructor with keywords. Unknown parameters are errors.
Use `COLA.construct(YourFilter; kwargs...)` to exercise identical parsing in tests.
The host attributes `name` and `filter` are reserved.

## Native tests and REPL

```sh
julia --project=/path/to/MyPhysics -e 'using Pkg; Pkg.test()'
julia --project=/path/to/MyPhysics
```

```julia
using COLA, MyPhysics
event = EventData()
particle = Particle()
set_energy!(momentum(particle), 5.0)
push!(particles(event), particle)
convert!(MyPhysics.MomentumScaler(scale=2), event)
@assert energy(momentum(first(particles(event)))) == 10
```

Use `Test` assertions in `test/runtests.jl`; no COLA runner or XML is required.
Optional Revise can be loaded before the user package for interactive development.
Embedded dispatch uses `invokelatest` so newly loaded package methods are visible.
Automatic file watching in the embedded process is not enabled.

Use the CMake-built or installed API package: it loads the native bindings from
its own `deps` directory.

## Run a calculation

Prepare one shared Julia environment for all packages used by a calculation:

```julia
using Pkg
Pkg.activate("/path/to/calculation")
Pkg.develop(path="/path/to/installed/share/COLA_Julia/julia/COLA")
Pkg.develop(path="/path/to/MyPhysics") # or Pkg.add for a registered/Git package
Pkg.instantiate()
Pkg.precompile()
```

Use Julia's dependency resolver and commit the calculation's Manifest.toml for
reproducibility. Conflicting dependency requirements must be resolved before running.
The embedded runtime never installs or upgrades packages.

```sh
export JULIA_PROJECT=/path/to/calculation
cola run --library=COLA_Julia --config=config.xml
```

```xml
<converter name="julia_converter"
           filter="MyPhysics.MomentumScaler"
           scale="2.5"/>
```

The generic factory names are `julia_generator`, `julia_converter`, and
`julia_writer`. The adapter uses Julia's normal environment selection: set
`JULIA_PROJECT` before starting the process, or leave it unset to use Julia's default
environment. There is no per-filter project setting. All Julia filters share the
runtime and load their packages dynamically from its environment.

`filter` has the form `Package.FilterType` or `Package.Submodule.FilterType`, with any
number of nested modules. The last component names the actual Julia type, which
must subtype the expected `Generator`, `Converter`, or `Writer`. It is resolved
without evaluating Julia code, then constructed using its parameter schema.
No separate `package` attribute is needed.

## Ownership and runtime

Events created in Julia have CxxWrap-managed native ownership. Filter arguments
are borrowed C++ objects valid only during the callback. Do not retain borrowed
events or references to their fields after returning. Retain an explicit `copy`
instead when independent storage is required.

Nested accessors return native references. Keep the owning event or particle alive
while using them (use `GC.@preserve` when working with detached references).
CxxWrap supplies the particle vector operations. Inserting a particle copies that
particle into the C++ vector; reading and mutating its fields does not copy the event.
Structural vector changes can invalidate previously obtained particle references.
Do not retain them across `push!`, `resize!`, or `empty!`.
These are borrowed-reference contracts, not checked handles.

The adapter owns one Julia runtime and calls Julia directly on the caller's thread.
Initialization, all filter calls, filter destruction, and runtime shutdown must happen
on that same thread (normally the main thread). There is no worker or task queue;
cross-thread use is unsupported and is not checked at runtime.
Each C++ filter handle retains its Julia object using CxxWrap's
persistent GC roots. Destruction calls `close!` and releases the root, including
when `close!` throws. Julia does not maintain an ID-to-filter dictionary.
The internal Julia adapter in `src/bridge.jl` resolves qualified types,
validates their kind, constructs filters using their schemas, dispatches callbacks,
and captures exceptions with backtraces. C++ provides the COLA interfaces, runtime
lifetime, and RAII ownership; there is no duplicate lookup or dispatch logic in C++.

Julia exceptions include their backtraces in C++ errors. This adapter must not initialize
a second runtime inside an already-running Julia process: a Julia session loads
only the bindings package. Shutdown finalizes Julia after all filter handles have
been destroyed. Loaded packages and the runtime remain resident for the process lifetime.

## Binding generation

```sh
cmake --build build --target COLA_JuliaBindings
```

Normal builds also build this target. CMake configures `bindings/wrapit.toml.in`
with the COLA header location and regenerates bindings when the headers,
configuration, or veto list change.
The veto list excludes copying aggregate getters. `JuliaBindings.cc` supplies
reference accessors and the LorentzVector anonymous-union fields that WrapIt misses.
Do not edit generated C++ by hand.

Tests cover Pkg.test, source loading, native event identity/mutation, metadata errors,
Julia exceptions, GC lifetimes, and loading through COLA's actual module loader.
