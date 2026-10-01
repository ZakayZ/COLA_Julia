#ifndef COLA_JULIA_JULIA_FILTERS_HH
#define COLA_JULIA_JULIA_FILTERS_HH

#include <COLA.hh>

#include <memory>
#include <string>
#include <unordered_map>

namespace cola::jl {

  template <FilterType Kind, bool Unsafe = false>
  class JuliaFilterHandle;

  class JuliaGenerator final : public VGenerator {
   public:
    explicit JuliaGenerator(const std::unordered_map<std::string, std::string>& metadata);

    ~JuliaGenerator() override;

    std::unique_ptr<EventData> operator()() override;

    inline static const std::string kName = "julia_generator";

   private:
    std::unique_ptr<JuliaFilterHandle<FilterType::kGenerator>> handle_;
  };

  class JuliaConverter final : public VConverter, public VTimedConverter {
   public:
    explicit JuliaConverter(const std::unordered_map<std::string, std::string>& metadata);

    ~JuliaConverter() override;

    std::unique_ptr<EventData> operator()(std::unique_ptr<EventData>&& event) override;

    std::uint64_t LastCallbackNanoseconds() const override;

    inline static const std::string kName = "julia_converter";

   private:
    std::unique_ptr<JuliaFilterHandle<FilterType::kConverter>> handle_;
  };

  class JuliaWriter final : public VWriter {
   public:
    explicit JuliaWriter(const std::unordered_map<std::string, std::string>& metadata);

    ~JuliaWriter() override;

    void operator()(std::unique_ptr<EventData>&& event) override;

    inline static const std::string kName = "julia_writer";

   private:
    std::unique_ptr<JuliaFilterHandle<FilterType::kWriter>> handle_;
  };

  class JuliaUnsafeGenerator final : public VGenerator {
   public:
    explicit JuliaUnsafeGenerator(const std::unordered_map<std::string, std::string>& metadata);

    ~JuliaUnsafeGenerator() override;

    std::unique_ptr<EventData> operator()() override;

    inline static const std::string kName = "julia_unsafe_generator";

   private:
    std::unique_ptr<JuliaFilterHandle<FilterType::kGenerator, true>> handle_;
  };

  class JuliaUnsafeConverter final : public VConverter, public VTimedConverter {
   public:
    explicit JuliaUnsafeConverter(const std::unordered_map<std::string, std::string>& metadata);

    ~JuliaUnsafeConverter() override;

    std::unique_ptr<EventData> operator()(std::unique_ptr<EventData>&& event) override;

    std::uint64_t LastCallbackNanoseconds() const override;

    inline static const std::string kName = "julia_unsafe_converter";

   private:
    std::unique_ptr<JuliaFilterHandle<FilterType::kConverter, true>> handle_;
  };

  using JuliaGeneratorFactory = GenericFactory<JuliaGenerator>;
  using JuliaConverterFactory = GenericFactory<JuliaConverter>;
  using JuliaWriterFactory = GenericFactory<JuliaWriter>;
  using JuliaUnsafeGeneratorFactory = GenericFactory<JuliaUnsafeGenerator>;
  using JuliaUnsafeConverterFactory = GenericFactory<JuliaUnsafeConverter>;
  using JuliaModule = GenericModule<JuliaGeneratorFactory, JuliaConverterFactory, JuliaWriterFactory,
                                    JuliaUnsafeGeneratorFactory, JuliaUnsafeConverterFactory>;

}  // namespace cola::jl

#endif
