using CxxWrap

source = dirname(@__DIR__)
build = joinpath(@__DIR__, "build")
julia = joinpath(Sys.BINDIR, Base.julia_exename())
options = ["-DBUILD_TESTING=OFF", "-DCMAKE_BUILD_TYPE=Release",
           "-DJlCxx_DIR=$(joinpath(CxxWrap.prefix_path(), "lib", "cmake", "JlCxx"))",
           "-DJulia_EXECUTABLE=$julia", "-DCMAKE_INSTALL_LIBDIR=lib"]

# Keep CMake's Julia subprocesses in Pkg's dependency environment.
withenv("JULIA_PROJECT" => dirname(Base.active_project())) do
    run(`cmake -S $source -B $build $options`)
    run(`cmake --build $build --config Release --target COLA_Julia`)
    run(`cmake --install $build --config Release`)
end

# Record the location only after installation succeeds.
prefix = read(joinpath(build, "cola-prefix.txt"), String)
open(joinpath(@__DIR__, "deps.jl"), "w") do io
    println(io, "const _native_prefix = ", repr(prefix))
end
