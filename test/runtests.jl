using Test, COLA
import COLA: position

struct RequiredParameterFilter <: Converter
    count::Int
end
RequiredParameterFilter(; count) = RequiredParameterFilter(count)
COLA.parameters(::Type{RequiredParameterFilter}) = (count=parameter(Int),)

struct CustomParameter end
Base.parse(::Type{CustomParameter}, value::AbstractString) = throw(DomainError(value, "custom parser failure"))
struct CustomParameterFilter <: Converter
    value::CustomParameter
end
CustomParameterFilter(; value) = CustomParameterFilter(value)
COLA.parameters(::Type{CustomParameterFilter}) = (value=parameter(CustomParameter),)

@testset "Type-based parameter schema" begin
    @test construct(RequiredParameterFilter; count="3").count == 3
    @test_throws ArgumentError construct(RequiredParameterFilter)
    @test_throws ArgumentError construct(RequiredParameterFilter; count="bad")
    @test_throws ArgumentError construct(RequiredParameterFilter; count=3, unknown=1)
    @test_throws DomainError construct(CustomParameterFilter; value="bad")
end

@testset "Native call boundary" begin
    @test COLA._invoke_with_error_capture(identity, "a successful string") == ("a successful string", nothing)
    value, error = COLA._invoke_with_error_capture(() -> throw(ArgumentError("test failure")))
    @test isnothing(value)
    @test occursin("test failure", error)
end

@testset "Qualified filter validation" begin
    for path in ("", "MyPhysics", ".Scaler", "MyPhysics.", "MyPhysics..Scaler", "MyPhysics.Scaler()")
        @test_throws ArgumentError COLA._resolve_filter_type(path)
    end
end

@testset "Native data, per-particle access" begin
    event = EventData()
    particle = Particle()
    set_pdg_code!(particle, Int32(211))
    set_particle_class!(particle, PRODUCED)
    set_energy!(momentum(particle), 5.0)
    set_x!(position(particle), 3.0)
    push!(particles(event), particle)
    @test length(particles(event)) == 1
    @test pdg_code(first(particles(event))) == 211
    @test particle_class(first(particles(event))) == PRODUCED
    @test x(position(first(particles(event)))) == 3.0
    set_energy!(momentum(first(particles(event))), 8.0)
    @test energy(momentum(first(particles(event)))) == 8.0
    @test energy(momentum(particle)) == 5.0 # std::vector push copies one particle.
    set_energy!(initial_state(event), 42.0)
    push!(initial_state_particles(initial_state(event)), particle)
    GC.gc()
    @test energy(initial_state(event)) == 42.0
    @test length(initial_state_particles(initial_state(event))) == 1
    @test_throws BoundsError particles(event)[2]
    empty!(particles(event))
    @test isempty(particles(event))
end

@testset "Event copying" begin
    source = EventData()
    set_energy!(initial_state(source), 3.0)
    snapshot = copy(source)
    set_energy!(initial_state(source), 4.0)
    @test energy(initial_state(snapshot)) == 3.0
end
