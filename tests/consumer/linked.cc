#include <COLA_Julia/JuliaFilters.hh>

#include <iostream>
#include <stdexcept>

int main() {
  try {
    cola::jl::JuliaGenerator generator({{"filter", "JuliaTestFilters.TestGenerator"}});
    cola::jl::JuliaConverter converter({{"filter", "JuliaTestFilters.ScaleConverter"}, {"scale", "2"}});
    cola::jl::JuliaWriter writer({{"filter", "JuliaTestFilters.TestWriter"}});
    auto event = generator();
    auto* original = event.get();
    event = converter(std::move(event));
    if (event.get() != original || event->particles.size() != 1 || event->particles.front().momentum.e != 10) {
      throw std::runtime_error("incorrect event conversion");
    }
    writer(std::move(event));
    std::cout << "Linked Julia pipeline passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
