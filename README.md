# COLA Julia

Julia bindings for [COLA](https://github.com/Spectator-matter-group-INR-RAS/COLA).
Write physics filters as Julia packages and use them in COLA calculations.

## Quick start

Requires an installed COLA discoverable by CMake, Julia, CMake and a C++20 compiler.
From a local checkout, install into COLA's writable native prefix:

```julia
using Pkg
Pkg.develop(path="/absolute/path/to/COLA_Julia")
Pkg.build("COLA")
```

Add your filter package to the same Julia environment:

```julia
Pkg.develop(path="/absolute/path/to/MyPhysics")
```

Run using the [COLA-PY](https://github.com/ZakayZ/COLA-PY) CLI:

```shell
export COLA_DIR="/path/to/cola/install"
cola run --config config.xml --library COLA_Julia
```

Example entry in your calculation XML:

```xml
<converter name="julia_converter" filter="MyPhysics.ScaleConverter" scale="2"/>
```

The default Julia environment is used unless `JULIA_PROJECT` is set.

## Write a filter

Subtype `COLA.Generator`, `COLA.Converter` or `COLA.Writer` and implement
`generate!`, `convert!` or `write!`. `generate!` returns a new `EventData`;
`convert!` receives an owned copy and returns the event to continue through the
calculation. A writer receives an owned copy. These events may be retained.

For the previous zero-copy, in-place contract, use `COLA.UnsafeGenerator` with
`generate!(filter, event)` or `COLA.UnsafeConverter` with `convert!(filter, event)`
and select `julia_unsafe_generator` or `julia_unsafe_converter` in XML. Borrowed
events and field references must not escape those callbacks.

See the [examples branch](https://github.com/ZakayZ/COLA_Julia/tree/examples/examples/julia-filters)
for complete packages and tests.

## Tests

```julia
using Pkg
Pkg.test("COLA")
Pkg.test("MyPhysics")
```

See [tests/consumer](tests/consumer) for C++ linking and dynamic-loading tests.
