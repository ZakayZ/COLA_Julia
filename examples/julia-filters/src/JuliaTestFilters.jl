module JuliaTestFilters

using COLA
import COLA: convert!, generate!, write!

struct TestGenerator <: Generator end

function generate!(::TestGenerator, event)
    state = initial_state(event)
    set_energy!(state, 42.0)

    particle = Particle()
    set_pdg_code!(particle, Int32(211))
    set_particle_class!(particle, PRODUCED)
    particle_momentum = momentum(particle)
    set_energy!(particle_momentum, 5.0)
    set_x!(particle_momentum, 1.0)
    set_y!(particle_momentum, 2.0)
    set_z!(particle_momentum, 3.0)
    push!(particles(event), particle)
    return nothing
end

struct ScaleConverter <: Converter
    scale::Float64
end

ScaleConverter(; scale::Real=1.0) = ScaleConverter(Float64(scale))
COLA.parameters(::Type{ScaleConverter}) = (scale=parameter(Float64; default=1.0),)

function convert!(converter::ScaleConverter, event)
    for particle in particles(event)
        vector = momentum(particle)
        set_energy!(vector, energy(vector) * converter.scale)
    end
    return nothing
end

struct ThrowingConverter <: Converter end
convert!(::ThrowingConverter, event) = error("intentional Julia filter failure")

# Weak references let the native integration test observe ownership without
# keeping these objects alive from Julia.
const lifetime_refs = WeakRef[]
const close_count = Ref(0)
mutable struct LifetimeConverter <: Converter
    fail_close::Bool
    calls::Int
end
function LifetimeConverter(; fail_close=false)
    filter = LifetimeConverter(fail_close, 0)
    push!(lifetime_refs, WeakRef(filter))
    return filter
end
COLA.parameters(::Type{LifetimeConverter}) = (fail_close=parameter(Bool; default=false),)
function convert!(filter::LifetimeConverter, event)
    GC.gc(true)
    filter.calls += 1
    set_energy!(initial_state(event), Float64(filter.calls))
end
function COLA.close!(filter::LifetimeConverter)
    close_count[] += 1
    filter.fail_close && error("intentional close failure")
    return nothing
end
struct LifetimeCheck <: Generator
    live::Int
    closed::Int
end
function LifetimeCheck(; live, closed)
    GC.gc(true)
    count(ref -> ref.value !== nothing, lifetime_refs) == live || error("incorrect live filter count")
    close_count[] == closed || error("incorrect close count")
    return LifetimeCheck(live, closed)
end
COLA.parameters(::Type{LifetimeCheck}) = (live=parameter(Int), closed=parameter(Int))

module Nested
module Inner
using COLA
using ...JuliaTestFilters: ScaleConverter
end
end

struct TestWriter <: Writer end

function write!(::TestWriter, event)
    energy(initial_state(event)) == 42.0 || error("writer received incorrect initial-state energy")
    length(particles(event)) == 1 || error("writer received incorrect particle count")
    energy(momentum(first(particles(event)))) == 10.0 || error("writer received unscaled momentum")
    return nothing
end

end
