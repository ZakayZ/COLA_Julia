module COLA

using CxxWrap

module Native
using CxxWrap, Libdl
function library_path()
    path = joinpath(@__DIR__, "..", "deps", "libCOLA_JuliaBindings." * Libdl.dlext)
    isfile(path) || error("COLA bindings not found at $path. Run Pkg.build(\"COLA\") or use the CMake-installed package.")
    return path
end
@wrapmodule(library_path, :define_cola_julia)
function __init__()
    @initcxx
end
end

const EventData = Native.cola!EventData
const EventInitialState = Native.cola!EventIniState
const Particle = Native.cola!Particle
const LorentzVector = Native.cola!LorentzVectorImpl{Float64}
const ParticleClass = Native.cola!ParticleClass
for (public_name, native_name) in (
    :PRODUCED => :cola!ParticleClass!kProduced,
    :ELASTIC_A => :cola!ParticleClass!kElasticA,
    :ELASTIC_B => :cola!ParticleClass!kElasticB,
    :NON_ELASTIC_A => :cola!ParticleClass!kNonelasticA,
    :NON_ELASTIC_B => :cola!ParticleClass!kNonelasticB,
    :SPECTATOR_A => :cola!ParticleClass!kSpectatorA,
    :SPECTATOR_B => :cola!ParticleClass!kSpectatorB,
)
    @eval const $public_name = Native.$native_name
    @eval export $public_name
end

for name in (:energy, :time, :x, :y, :z, :pdg_code, :pdg_code_a, :pdg_code_b,
             :pz_a, :pz_b, :sect_nn, :num_coll, :num_coll_pp, :num_coll_pn,
             :num_coll_nn, :num_part, :num_part_a, :num_part_b, :phi_rot_a,
             :theta_rot_a, :phi_rot_b, :theta_rot_b)
    setter = Symbol("set_", name, "!")
    native_setter = Symbol(name, "!")
    @eval const $name = Native.$name
    @eval const $setter = Native.$native_setter
    @eval export $name, $setter
end
const impact_parameter = Native.b
const set_impact_parameter! = Native.b!
const particle_class = Native.p_class
const set_particle_class! = Native.p_class!
const initial_state = Native.initial_state
particles(event) = Native.particles(event)[]
initial_state_particles(state) = Native.initial_state_particles(state)[]
# Dereference the container, not individual pointer offsets. Add bounds checks
# for our particle specialization; iteration/storage stay in CxxWrap's StdVector.
function Base.getindex(vector::CxxWrap.StdLib.StdVector{Particle}, index::Int)
    @boundscheck checkbounds(vector, index)
    return CxxWrap.StdLib.cxxgetindex(vector, index)[]
end
const momentum = Native.momentum
const position = Native.position

abstract type Generator end
abstract type Converter end
abstract type Writer end
function generate! end
function convert! end
function write! end
close!(::Union{Generator,Converter,Writer}) = nothing

struct Required end
struct Parameter{T,D}
    default::D
end
parameter(::Type{T}; default=Required()) where {T} = Parameter{T,typeof(default)}(default)
"""Keyword parameter schema for a filter type; empty by default."""
parameters(::Type) = (;)

_parse(::Type{String}, value::AbstractString) = String(value)
_parse(::Type{T}, value::AbstractString) where {T} = parse(T, value)
_parse(::Type{T}, value) where {T} = convert(T, value)
function _parameter_value(name, spec::Parameter{T}, metadata) where {T}
    if haskey(metadata, name)
        try
            return _parse(T, metadata[name])
        catch
            throw(ArgumentError("invalid value for parameter '$name': $(repr(metadata[name])); expected $T"))
        end
    end
    spec.default isa Required && throw(ArgumentError("missing required parameter '$name'"))
    return _parse(T, spec.default)
end

"""Construct a filter type with the same keyword conversion used by COLA XML."""
function construct(T::Type{<:Union{Generator,Converter,Writer}}; kwargs...)
    schema = parameters(T)
    unknown = setdiff(keys(kwargs), keys(schema))
    isempty(unknown) || throw(ArgumentError("unknown parameters: $(join(unknown, ", "))"))
    values = (; (name => _parameter_value(name, p, kwargs) for (name, p) in pairs(schema))...)
    return T(; values...)
end

include("bridge.jl")

export EventData, EventInitialState, Particle, ParticleClass, LorentzVector,
       Generator, Converter, Writer, generate!, convert!, write!, close!,
       parameters, parameter, construct,
       particles, initial_state, initial_state_particles, position, momentum,
       particle_class, set_particle_class!, impact_parameter, set_impact_parameter!
end
