source = dirname(@__DIR__)
build = joinpath(@__DIR__, "build")
julia = joinpath(Sys.BINDIR, Base.julia_exename())
options = ["-DBUILD_TESTING=OFF", "-DCMAKE_BUILD_TYPE=Release",
           "-DJulia_EXECUTABLE=$julia", "-DCOLA_JULIA_PACKAGE_DIR=$source"]
if haskey(ENV, "COLA_DIR")
    push!(options, "-DCOLA_DIR=$(ENV["COLA_DIR"])")
end

# Keep CMake's Julia subprocesses in Pkg's dependency environment.
withenv("JULIA_PROJECT" => dirname(Base.active_project())) do
    run(`cmake -S $source -B $build $options`)
    run(`cmake --build $build --config Release --target COLA_Julia`)
end
