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
      struct ValueDeleter {
        void operator()(jl_value_t* value) const noexcept { jlcxx::unprotect_from_gc(value); }
      };

      using Value = std::unique_ptr<jl_value_t, ValueDeleter>;

      static Runtime& Instance() {
        static Runtime runtime;
        return runtime;
      }

      ~Runtime() { jl_atexit_hook(0); }

      template <typename... Args>
      auto Invoke(const char* name, Args&&... args) {
        jlcxx::JuliaFunction function(name, "COLA");
        jlcxx::JuliaFunction guard("_invoke_with_error_capture", "COLA");
        auto* result = guard(function.pointer(), std::forward<Args>(args)...);
        if (result == nullptr) {
          throw std::runtime_error("Julia invocation failed (see Julia diagnostic)");
        }
        JL_GC_PUSH1(&result);
        if (auto* error = jl_get_nth_field(result, 1); error != jl_nothing) {
          // Unwind the Julia root frame before propagating a C++ exception.
          JL_GC_POP();
          throw std::runtime_error(jl_string_ptr(error));
        }
        auto* value = jl_get_nth_field(result, 0);
        jlcxx::protect_from_gc(value);
        JL_GC_POP();
        return Value(value);
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

  template <FilterType Kind, bool Unsafe>
  class JuliaFilterHandle {
   public:
    explicit JuliaFilterHandle(const Metadata& metadata) : object_(Create(metadata)) {}

    std::unique_ptr<EventData> Generate() const {
      return CopyEvent(Runtime::Instance().Invoke("_generate_event", object_.get()).get());
    }

    std::unique_ptr<EventData> Convert(const EventData& event) const {
      auto result = Runtime::Instance().Invoke("_convert_event_timed", object_.get(), event);
      last_callback_nanoseconds_ = jl_unbox_uint64(jl_get_nth_field(result.get(), 1));
      return CopyEvent(jl_get_nth_field(result.get(), 0));
    }

    void Write(const EventData& event) const { Runtime::Instance().Invoke("_write_event", object_.get(), event); }

    void RunUnsafe(EventData& event) const {
      if constexpr (Kind == FilterType::kConverter) {
        auto result = Runtime::Instance().Invoke("_process_unsafe_event_timed", object_.get(), event);
        last_callback_nanoseconds_ = jl_unbox_uint64(result.get());
      } else {
        Runtime::Instance().Invoke("_process_unsafe_event", object_.get(), event);
      }
    }

    std::uint64_t LastCallbackNanoseconds() const { return last_callback_nanoseconds_; }

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
      if constexpr (Unsafe) {
        return FilterPtr(Runtime::Instance()
                             .Invoke("_create_unsafe_filter", static_cast<std::uint8_t>(Kind), keys, values)
                             .release());
      }
      return FilterPtr(
          Runtime::Instance().Invoke("_create_filter", static_cast<std::uint8_t>(Kind), keys, values).release());
    }

    static auto CopyEvent(jl_value_t* value) { return std::make_unique<EventData>(*jlcxx::unbox<EventData*>(value)); }

    FilterPtr object_;
    mutable std::uint64_t last_callback_nanoseconds_{};
  };

  JuliaGenerator::JuliaGenerator(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kGenerator>>(metadata)) {}

  JuliaGenerator::~JuliaGenerator() = default;

  std::unique_ptr<EventData> JuliaGenerator::operator()() { return handle_->Generate(); }

  JuliaConverter::JuliaConverter(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kConverter>>(metadata)) {}

  JuliaConverter::~JuliaConverter() = default;

  std::unique_ptr<EventData> JuliaConverter::operator()(std::unique_ptr<EventData>&& event) {
    return handle_->Convert(*event);
  }

  std::uint64_t JuliaConverter::LastCallbackNanoseconds() const { return handle_->LastCallbackNanoseconds(); }

  JuliaWriter::JuliaWriter(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kWriter>>(metadata)) {}

  JuliaWriter::~JuliaWriter() = default;

  void JuliaWriter::operator()(std::unique_ptr<EventData>&& event) { handle_->Write(*event); }

  JuliaUnsafeGenerator::JuliaUnsafeGenerator(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kGenerator, true>>(metadata)) {}

  JuliaUnsafeGenerator::~JuliaUnsafeGenerator() = default;

  std::unique_ptr<EventData> JuliaUnsafeGenerator::operator()() {
    auto event = std::make_unique<EventData>();
    handle_->RunUnsafe(*event);
    return event;
  }

  JuliaUnsafeConverter::JuliaUnsafeConverter(const Metadata& metadata)
      : handle_(std::make_unique<JuliaFilterHandle<FilterType::kConverter, true>>(metadata)) {}

  JuliaUnsafeConverter::~JuliaUnsafeConverter() = default;

  std::unique_ptr<EventData> JuliaUnsafeConverter::operator()(std::unique_ptr<EventData>&& event) {
    handle_->RunUnsafe(*event);
    return std::move(event);
  }

  std::uint64_t JuliaUnsafeConverter::LastCallbackNanoseconds() const { return handle_->LastCallbackNanoseconds(); }
}  // namespace cola::jl
