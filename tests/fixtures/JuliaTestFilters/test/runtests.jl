using Test, COLA, JuliaTestFilters

@testset "Julia integration fixtures" begin
    event = generate!(JuliaTestFilters.TestGenerator())
    @test energy(initial_state(event)) == 42.0
    T = JuliaTestFilters.ScaleConverter
    filter = construct(T; scale="2")
    event = convert!(filter, event)
    @test energy(momentum(first(particles(event)))) == 10.0
    @test isnothing(write!(JuliaTestFilters.TestWriter(), event))
    @test_throws ArgumentError construct(T; scale="bad")
    @test_throws ArgumentError construct(T; unknown=1)
    @test construct(T).scale == 1.0
    @test construct(T; scale=3).scale == 3.0
    @test construct(JuliaTestFilters.TestGenerator) isa Generator
    @test construct(JuliaTestFilters.UnsafeTestGenerator) isa UnsafeGenerator
    @test_throws ArgumentError construct(JuliaTestFilters.TestGenerator; unknown=1)
end
