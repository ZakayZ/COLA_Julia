#include <COLA_Julia/JuliaFilters.hh>

extern "C" cola::VModule* LoadCOLAModule() { return new cola::jl::JuliaModule(); }
