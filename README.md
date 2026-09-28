# COLA Julia

Julia wrapper for [COLA](https://github.com/Spectator-matter-group-INR-RAS/COLA).
Write generators, converters and writers as ordinary Julia packages, test them
with `Pkg.test()`, and load them into a COLA calculation.

## Installation

Requires an existing COLA installation, Julia, CMake and a C++20 compiler.
From a local checkout:

```julia
using Pkg
Pkg.develop(path="/absolute/path/to/COLA_Julia")
Pkg.build("COLA")
```

Pkg resolves CxxWrap and WrapIt, then CMake finds the installed COLA package and
installs native libraries, headers and the CMake package into the prefix exported
by COLA. COLA must be discoverable through CMake's standard package search.
The prefix must
be writable; COLA itself and system tools are not installed automatically.

The native installation is shared between Julia environments and is not removed
by `Pkg.rm("COLA")`. Use the same native CxxWrap artifact for building and running;
rebuild after changing Julia or native dependencies. `COLA.prefix_path()` returns
the recorded installation prefix.

## Usage

### Julia packages

Add `COLA` as a dependency of your Julia package. Subtype `COLA.Generator`,
`COLA.Converter` or `COLA.Writer` and implement `generate!`, `convert!` or
`write!`. Callbacks modify the supplied event in place. Optional `close!`
handles cleanup.

```julia
using COLA

struct ScaleConverter <: COLA.Converter
    scale::Float64
end
ScaleConverter(; scale=1.0) = ScaleConverter(Float64(scale))
COLA.parameters(::Type{ScaleConverter}) = (scale=COLA.parameter(Float64; default=1.0),)

function COLA.convert!(filter::ScaleConverter, event)
    for particle in COLA.particles(event)
        p = COLA.momentum(particle)
        COLA.set_energy!(p, COLA.energy(p) * filter.scale)
    end
end
```

Place this code in your package module, for example `MyPhysics`.
Use `COLA.construct(ScaleConverter; scale="2")` to test parameter parsing.
See [examples/julia-filters](examples/julia-filters) for generators, writers and tests.

Borrowed events and field references must not outlive their callback. Changes
to particle-vector storage can invalidate particle references. Embedded Julia
calls, filter destruction and shutdown must run on the initializing thread.

### Run

Install `COLA` and your filter packages in the calculation's Julia environment.
The `cola` command below is provided by [COLA-PY](https://github.com/ZakayZ/COLA-PY):

```shell
export COLA_DIR="/absolute/path/to/cola/install"
export JULIA_PROJECT="/absolute/path/to/calculation"
cola run --library=COLA_Julia --config=config.xml
```

Example converter entry in the calculation configuration:

```xml
<converter name="julia_converter" filter="MyPhysics.ScaleConverter" scale="2"/>
```

The factory names are `julia_generator`, `julia_converter` and `julia_writer`.
All filters share one runtime and environment. Without `JULIA_PROJECT`,
Julia uses its default environment.

### C++

Use the installation prefix provided by COLA:

```cmake
find_package(COLA CONFIG REQUIRED)
find_package(COLA_Julia CONFIG REQUIRED HINTS "${COLA_DIR}")
target_link_libraries(my_calculation PRIVATE COLA_Julia::COLA_Julia)
```

Alternatively, load the plugin with `cola::LoadModule("COLA_Julia")`, using
`COLA_DIR` to locate the installation.

## Develop

```julia
using Pkg
Pkg.test("COLA")
```

For installed-library integration tests, add `examples/julia-filters` to the
same environment with `Pkg.develop(path="examples/julia-filters")`, then run:

```shell
cmake -S tests/consumer -B build-consumer
cmake --build build-consumer
JULIA_PROJECT=/path/to/calculation ctest --test-dir build-consumer --output-on-failure
```

These tests check both linking through the exported CMake target and dynamic
plugin loading. Apple Silicon macOS has been tested; Linux remains unverified.
