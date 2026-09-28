#include "JuliaFilters.hh"

#include <jlcxx/functions.hpp>
#include <jlcxx/stl.hpp>  // IWYU pragma: keep; registers vector argument conversions
#include <julia.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cola::jl {
  namespace {
    using Metadata = std::unordered_map<std::string, std::string>;

    // All embedding calls, including finalization, execute on the initializing thread.
    class Runtime {
     public:
      static Runtime& Instance() {
        static Runtime runtime;
        return runtime;
      }

      ~Runtime() { jl_atexit_hook(0); }

      template <typename... Args>
      jl_value_t* Invoke(const char* name, Args&&... args) {
        jlcxx::JuliaFunction function(name, "COLA");
        jlcxx::JuliaFunction guard("_invoke_with_error_capture", "COLA");
        auto* result = guard(function.pointer(), std::forward<Args>(args)...);
        if (result == nullptr) {
          throw std::runtime_error("Julia invocation failed (see Julia diagnostic)");
        }
        JL_GC_PUSH1(&result);
        auto* error = jl_get_nth_field(result, 1);
        if (error != jl_nothing) {
          // Unwind the Julia root frame before propagating a C++ exception.
          JL_GC_POP();
          throw std::runtime_error(jl_string_ptr(error));
        }
        auto* value = jl_get_nth_field(result, 0);
        JL_GC_POP();
        return value;
      }

     private:
      Runtime() {
        if (jl_is_initialized()) {
          throw std::runtime_error("COLA embedding requires ownership of the Julia runtime");
        }
        jl_init();
        try {
          jl_eval_string("using COLA");
          if (jl_exception_occurred() != nullptr) {
            jl_call2(jl_get_function(jl_base_module, "showerror"), jl_stderr_obj(), jl_exception_occurred());
            throw std::runtime_error("Cannot load COLA; install it in the active Julia environment before running");
          }
        } catch (...) {
          jl_atexit_hook(1);
          throw;
        }
      }
    };
  }  // namespace

  template <FilterType Kind>
  class JuliaFilterHandle {
   public:
    explicit JuliaFilterHandle(const Metadata& metadata) : object_(Create(metadata)) {}

    void Run(EventData& event) const { Runtime::Instance().Invoke("_process_event", object_.get(), event); }

   private:
    struct FilterDeleter {
      void operator()(jl_value_t* object) const noexcept {
        try {
          Runtime::Instance().Invoke("close!", object);
        } catch (const std::exception& error) {
          std::fprintf(stderr, "COLA Julia filter close failed: %s\n", error.what());
        } catch (...) {
          std::fprintf(stderr, "COLA Julia filter close failed: unknown exception\n");
        }
        // Julia owns the storage; release the root even when close! fails.
        jlcxx::unprotect_from_gc(object);
      }
    };
    using FilterPtr = std::unique_ptr<jl_value_t, FilterDeleter>;

    static FilterPtr Create(const Metadata& metadata) {
      std::vector<std::string> keys;
      std::vector<std::string> values;
      for (const auto& [key, value] : metadata) {
        keys.push_back(key);
        values.push_back(value);
      }
      auto* object = Runtime::Instance().Invoke("_create_filter", static_cast<std::uint8_t>(Kind), keys, values);
      jlcxx::protect_from_gc(object);
      return FilterPtr(object);
    }

    FilterPtr object_;
  };

  JuliaGenerator::JuliaGenerator(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kGenerator>>(metadata)) {}

  JuliaGenerator::~JuliaGenerator() = default;

  std::unique_ptr<EventData> JuliaGenerator::operator()() {
    auto event = std::make_unique<EventData>();
    handle_->Run(*event);
    return event;
  }

  JuliaConverter::JuliaConverter(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kConverter>>(metadata)) {}

  JuliaConverter::~JuliaConverter() = default;

  std::unique_ptr<EventData> JuliaConverter::operator()(std::unique_ptr<EventData>&& event) {
    handle_->Run(*event);
    return std::move(event);
  }

  JuliaWriter::JuliaWriter(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kWriter>>(metadata)) {}

  JuliaWriter::~JuliaWriter() = default;

  void JuliaWriter::operator()(std::unique_ptr<EventData>&& event) { handle_->Run(*event); }
}  // namespace cola::jl
