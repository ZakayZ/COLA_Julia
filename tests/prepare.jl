using Pkg
length(ARGS) == 3 || error("usage: prepare.jl ENVIRONMENT API_PACKAGE FILTER_PACKAGE")
Pkg.activate(abspath(ARGS[1]))
Pkg.develop([PackageSpec(path=abspath(ARGS[2])), PackageSpec(path=abspath(ARGS[3]))])
Pkg.instantiate(; update_registry=false)
