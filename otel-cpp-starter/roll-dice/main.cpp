#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

#include "opentelemetry/common/key_value_iterable_view.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter_options.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/metrics/sync_instruments.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/provider.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
#include "opentelemetry/sdk/metrics/provider.h"
#include "opentelemetry/sdk/metrics/view/view_registry_factory.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <utility>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace logs_api = opentelemetry::logs;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace metrics_api = opentelemetry::metrics;
namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace resource_sdk = opentelemetry::sdk::resource;
namespace otlp = opentelemetry::exporter::otlp;

namespace {

struct PreparationStep {
  const char* span_name;
  int duration_ms;
};

struct MenuItem {
  const char* id;
  const char* display_name;
  std::span<const PreparationStep> steps;
};

constexpr PreparationStep kCoffeeSteps[] = {
    {"GrindCoffeeBeans", 250},
    {"BrewCoffee", 450},
    {"PourCoffee", 150},
};

constexpr PreparationStep kLatteSteps[] = {
    {"GrindCoffeeBeans", 250},
    {"BrewEspresso", 400},
    {"SteamMilk", 350},
    {"PourLatte", 150},
};

constexpr PreparationStep kDessertSteps[] = {
    {"PrepareDessert", 250},
    {"BakeDessert", 600},
    {"PlateDessert", 150},
};

constexpr std::array<MenuItem, 3> kMenuItems = {{
    {"coffee", "Coffee", kCoffeeSteps},
    {"latte", "Latte", kLatteSteps},
    {"dessert", "Dessert", kDessertSteps},
}};

std::shared_ptr<trace_sdk::TracerProvider> tracer_provider;
std::shared_ptr<logs_sdk::LoggerProvider> logger_provider;
std::shared_ptr<metrics_sdk::MeterProvider> meter_provider;
opentelemetry::nostd::unique_ptr<metrics_api::Counter<std::uint64_t>>
    order_counter;
opentelemetry::nostd::unique_ptr<metrics_api::Histogram<double>>
    order_duration;

const MenuItem* FindMenuItem(const std::string& item_id) {
  for (const auto& item : kMenuItems) {
    if (item_id == item.id) {
      return &item;
    }
  }
  return nullptr;
}

resource_sdk::Resource CreateResource() {
  return resource_sdk::Resource::Create({
      {"service.name", "coffee-shop"},
      {"service.version", "1.0.0"},
  });
}

void InitTracer(const resource_sdk::Resource& resource) {
  otlp::OtlpHttpExporterOptions options;
  options.url = "http://localhost:4318/v1/traces";

  auto exporter = otlp::OtlpHttpExporterFactory::Create(options);
  auto processor =
      trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

  tracer_provider = trace_sdk::TracerProviderFactory::Create(
      std::move(processor), resource);
  trace_api::Provider::SetTracerProvider(tracer_provider);
}

void CleanupTracer() {
  if (tracer_provider) {
    tracer_provider->ForceFlush();
    tracer_provider->Shutdown();
  }

  std::shared_ptr<trace_api::TracerProvider> none;
  trace_api::Provider::SetTracerProvider(none);
  tracer_provider.reset();
}

void InitLogger(const resource_sdk::Resource& resource) {
  otlp::OtlpHttpLogRecordExporterOptions options;
  options.url = "http://localhost:4318/v1/logs";

  auto exporter = otlp::OtlpHttpLogRecordExporterFactory::Create(options);
  auto processor =
      logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
  logger_provider = logs_sdk::LoggerProviderFactory::Create(
      std::move(processor), resource);

  std::shared_ptr<logs_api::LoggerProvider> api_provider = logger_provider;
  logs_sdk::Provider::SetLoggerProvider(api_provider);
}

void CleanupLogger() {
  if (logger_provider) {
    logger_provider->ForceFlush();
    logger_provider->Shutdown();
  }

  std::shared_ptr<logs_api::LoggerProvider> none;
  logs_sdk::Provider::SetLoggerProvider(none);
  logger_provider.reset();
}

void InitMetrics(const resource_sdk::Resource& resource) {
  otlp::OtlpHttpMetricExporterOptions options;
  options.url = "http://localhost:4318/v1/metrics";

  auto exporter = otlp::OtlpHttpMetricExporterFactory::Create(options);
  metrics_sdk::PeriodicExportingMetricReaderOptions reader_options;
  reader_options.export_interval_millis = std::chrono::milliseconds(1000);
  reader_options.export_timeout_millis = std::chrono::milliseconds(500);

  auto reader = metrics_sdk::PeriodicExportingMetricReaderFactory::Create(
      std::move(exporter), reader_options);
  auto views = metrics_sdk::ViewRegistryFactory::Create();
  auto context =
      metrics_sdk::MeterContextFactory::Create(std::move(views), resource);
  context->AddMetricReader(std::move(reader));

  auto provider =
      metrics_sdk::MeterProviderFactory::Create(std::move(context));
  meter_provider =
      std::shared_ptr<metrics_sdk::MeterProvider>(std::move(provider));

  std::shared_ptr<metrics_api::MeterProvider> api_provider = meter_provider;
  metrics_sdk::Provider::SetMeterProvider(api_provider);

  auto meter = api_provider->GetMeter("coffee-shop-meter", "1.0.0");
  order_counter = meter->CreateUInt64Counter(
      "coffee_shop.orders", "Number of completed coffee shop orders", "1");
  order_duration = meter->CreateDoubleHistogram(
      "coffee_shop.order.duration", "Coffee shop order preparation time",
      "ms");
}

void CleanupMetrics() {
  if (meter_provider) {
    meter_provider->ForceFlush();
    meter_provider->Shutdown();
  }

  order_counter.reset();
  order_duration.reset();
  std::shared_ptr<metrics_api::MeterProvider> none;
  metrics_sdk::Provider::SetMeterProvider(none);
  meter_provider.reset();
}

void InitTelemetry() {
  const auto resource = CreateResource();
  InitTracer(resource);
  InitLogger(resource);
  InitMetrics(resource);
}

void CleanupTelemetry() {
  CleanupMetrics();
  CleanupLogger();
  CleanupTracer();
}

void LogPreparationStep(const MenuItem& item, const PreparationStep& step,
                        std::size_t step_number,
                        const trace_api::SpanContext& span_context) {
  auto logger = logs_api::Provider::GetLoggerProvider()->GetLogger(
      "coffee-shop-logger", "coffee-shop");
  const std::string message =
      "Preparing " + std::string(step.span_name);
  const std::map<std::string, std::string> attributes = {
      {"order.item", item.id},
      {"process.step.name", step.span_name},
      {"process.step.number", std::to_string(step_number)},
  };

  logger->Info(opentelemetry::nostd::string_view{message},
               span_context.trace_id(), span_context.span_id(),
               span_context.trace_flags(), attributes);
}

void RecordOrderMetrics(const MenuItem& item, double duration_ms) {
  if (!order_counter || !order_duration) {
    return;
  }

  const std::map<std::string, std::string> attributes = {
      {"order.item", item.id},
  };
  const auto context = opentelemetry::context::RuntimeContext::GetCurrent();
  order_counter->Add(1, attributes, context);
  order_duration->Record(duration_ms, attributes, context);
}

void PrepareOrder(const MenuItem& item) {
  auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
      "coffee-shop-tracer");

  auto order_span = tracer->StartSpan("PrepareOrder");
  auto order_scope = trace_api::Scope(order_span);
  const auto order_started = std::chrono::steady_clock::now();
  order_span->SetAttribute("order.item", item.id);
  order_span->SetAttribute("order.item.name", item.display_name);

  for (std::size_t index = 0; index < item.steps.size(); ++index) {
    const auto& step = item.steps[index];
    auto step_span = tracer->StartSpan(step.span_name);
    auto step_scope = trace_api::Scope(step_span);

    step_span->SetAttribute("order.item", item.id);
    step_span->SetAttribute("process.step.number",
                            static_cast<int>(index + 1));
    LogPreparationStep(item, step, index + 1, step_span->GetContext());
    std::this_thread::sleep_for(
        std::chrono::milliseconds(step.duration_ms));
    step_span->End();
  }

  const double duration_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - order_started)
          .count();
  RecordOrderMetrics(item, duration_ms);
  order_span->SetAttribute("order.status", "complete");
  order_span->SetAttribute("order.duration_ms", duration_ms);
  order_span->End();
}

}  // namespace

class OrderHandler : public oatpp::web::server::HttpRequestHandler {
 public:
  std::shared_ptr<OutgoingResponse> handle(
      const std::shared_ptr<IncomingRequest>& request) override {
    const auto item_parameter = request->getPathVariable("item");
    if (!item_parameter) {
      return ResponseFactory::createResponse(
          Status::CODE_400, R"({"error":"missing menu item"})");
    }

    const std::string item_id = item_parameter->c_str();
    const MenuItem* item = FindMenuItem(item_id);
    if (item == nullptr) {
      return ResponseFactory::createResponse(
          Status::CODE_404, R"({"error":"unknown menu item"})");
    }

    OATPP_LOGI("Coffee Shop", "Preparing %s", item->display_name);
    PrepareOrder(*item);
    OATPP_LOGI("Coffee Shop", "%s complete", item->display_name);

    const std::string response =
        "{\"status\":\"complete\",\"item\":\"" + item_id + "\"}";
    return ResponseFactory::createResponse(Status::CODE_200,
                                           response.c_str());
  }
};

void RunServer() {
  auto router = oatpp::web::server::HttpRouter::createShared();
  router->route("POST", "/order/{item}",
                std::make_shared<OrderHandler>());

  auto connection_handler =
      oatpp::web::server::HttpConnectionHandler::createShared(router);
  auto connection_provider =
      oatpp::network::tcp::server::ConnectionProvider::createShared(
          {"localhost", 8080, oatpp::network::Address::IP_4});

  oatpp::network::Server server(connection_provider, connection_handler);
  OATPP_LOGI(
      "Coffee Shop", "Server running on port %s",
      static_cast<const char*>(
          connection_provider->getProperty("port").getData()));
  server.run();
}

int main() {
  oatpp::base::Environment::init();
  InitTelemetry();
  RunServer();
  CleanupTelemetry();
  oatpp::base::Environment::destroy();
  return 0;
}
