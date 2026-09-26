#include <COLA.hh>

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {
  using Metadata = std::unordered_map<std::string, std::string>;
  void Require(bool condition, const char* message) {
    if (!condition) {
      throw std::runtime_error(message);
    }
  }
  template <typename F>
  void ExpectError(F&& operation, const std::string& expected) {
    try {
      operation();
    } catch (const std::exception& error) {
      Require(std::string(error.what()).find(expected) != std::string::npos, error.what());
      return;
    }
    throw std::runtime_error("Expected error: " + expected);
  }
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2) {
    return 2;
  }
  try {
    auto module = cola::LoadModule("COLA_Julia", std::string(argv[1]));
    auto factories = module->GetModuleFilters();
    const auto metadata = [&](std::string name) { return Metadata{{"filter", "JuliaTestFilters." + name}}; };
    auto generator = factories.at("julia_generator")->Create(metadata("TestGenerator"));
    auto event = (*dynamic_cast<cola::VGenerator*>(generator.get()))();
    Require(event && event->ini_state.energy == 42 && event->particles.size() == 1, "generator lost data");
    auto config = metadata("ScaleConverter");
    config["name"] = "julia_converter";  // Host metadata is not a constructor keyword.
    config["scale"] = "2";
    auto converter = factories.at("julia_converter")->Create(config);
    auto* native_address = event.get();
    event = (*dynamic_cast<cola::VConverter*>(converter.get()))(std::move(event));
    Require(event.get() == native_address, "converter copied the event");
    Require(event->particles.front().momentum.e == 10, "converter did not mutate native momentum");
    auto writer = factories.at("julia_writer")->Create(metadata("TestWriter"));
    (*dynamic_cast<cola::VWriter*>(writer.get()))(std::move(event));
    Require(!event, "writer did not consume the event");

    auto nested_config = metadata("Nested.Inner.ScaleConverter");
    nested_config["scale"] = "3";
    auto nested = factories.at("julia_converter")->Create(nested_config);
    event = (*dynamic_cast<cola::VGenerator*>(generator.get()))();
    event = (*dynamic_cast<cola::VConverter*>(nested.get()))(std::move(event));
    Require(event->particles.front().momentum.e == 15, "nested filter did not scale momentum");
    ExpectError([&] { factories.at("julia_converter")->Create(metadata("ScaleConverter.ScaleConverter")); },
                "not a module");

    config["scale"] = "invalid";
    ExpectError([&] { factories.at("julia_converter")->Create(config); }, "invalid value");
    ExpectError([&] { factories.at("julia_writer")->Create(metadata("ScaleConverter")); }, "wrong kind");
    ExpectError([&] { factories.at("julia_converter")->Create(metadata("missing")); }, "missing");
    ExpectError([&] { factories.at("julia_converter")->Create(metadata("Nested")); }, "must name a type");
    for (const auto* filter : {"JuliaTestFilters", ".ScaleConverter", "JuliaTestFilters.",
                               "JuliaTestFilters..ScaleConverter", "JuliaTestFilters.ScaleConverter()"}) {
      auto invalid = metadata("ScaleConverter");
      invalid["filter"] = filter;
      ExpectError([&] { factories.at("julia_converter")->Create(invalid); }, "expected Package.filter");
    }
    auto missing_filter = metadata("ScaleConverter");
    missing_filter.erase("filter");
    ExpectError([&] { factories.at("julia_converter")->Create(missing_filter); }, "requires 'filter'");
    missing_filter["filter"] = "";
    ExpectError([&] { factories.at("julia_converter")->Create(missing_filter); }, "requires 'filter'");
    auto unknown_parameter = metadata("ScaleConverter");
    unknown_parameter["unknown"] = "1";
    ExpectError([&] { factories.at("julia_converter")->Create(unknown_parameter); }, "unknown parameters");
    auto throwing = factories.at("julia_converter")->Create(metadata("ThrowingConverter"));
    event = (*dynamic_cast<cola::VGenerator*>(generator.get()))();
    ExpectError([&] { (*dynamic_cast<cola::VConverter*>(throwing.get()))(std::move(event)); },
                "intentional Julia filter failure");
    // A failed callback must not poison subsequent invocations.
    event = (*dynamic_cast<cola::VGenerator*>(generator.get()))();
    Require(event->particles.size() == 1, "runtime failed after exception");

    const auto check_lifetime = [&](int live, int closed) {
      auto config = metadata("LifetimeCheck");
      config["live"] = std::to_string(live);
      config["closed"] = std::to_string(closed);
      auto check = factories.at("julia_generator")->Create(config);
    };
    auto first = factories.at("julia_converter")->Create(metadata("LifetimeConverter"));
    auto close_config = metadata("LifetimeConverter");
    close_config["fail_close"] = "true";
    auto second = factories.at("julia_converter")->Create(close_config);
    check_lifetime(2, 0);  // Full GC while only C++ handles retain the objects.
    for (int call = 1; call <= 3; ++call) {
      event = (*dynamic_cast<cola::VConverter*>(first.get()))(std::move(event));
      Require(event->ini_state.energy == call, "Julia filter state lost during GC");
    }
    first.reset();
    check_lifetime(1, 1);
    second.reset();  // A failing close must still release the GC root.
    check_lifetime(0, 2);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
