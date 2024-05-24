#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <Windows.h>
#include <initguid.h>
#include <cfgmgr32.h>

//
// Driver-shared types
// 
#include <DsHidMini/ScpTypes.h>
#include <DsHidMini/Ds3Types.h>
#include <DsHidMini/dshmguid.h>

//
// STL
// 
#include <optional>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <array>
#include <format>

//
// Abseil
// 
#include <absl/cleanup/cleanup.h>
#include <absl/strings/match.h>

//
// OpenTelemetry
// 
#if defined(SCPLIB_ENABLE_TELEMETRY)
#define HAVE_ABSEIL // fixes redefinitions of absl types
// gRPC exporter
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h>

#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/resource/semantic_conventions.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>

namespace trace			= opentelemetry::trace;
namespace nostd			= opentelemetry::nostd;
namespace sdktrace		= opentelemetry::sdk::trace;
namespace otlp			= opentelemetry::exporter::otlp;
namespace sdkresource	= opentelemetry::sdk::resource;

// gRPC exporter
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_options.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/common/global_log_handler.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

namespace sdklogs		= opentelemetry::sdk::logs;
namespace logs			= opentelemetry::logs;

#include <opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_options.h>

#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/sdk/metrics/aggregation/default_aggregation.h>
#include <opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/meter.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/metrics/push_metric_exporter.h>
#include <opentelemetry/sdk/metrics/view/instrument_selector_factory.h>
#include <opentelemetry/sdk/metrics/view/meter_selector_factory.h>
#include <opentelemetry/sdk/metrics/view/view_factory.h>

namespace sdkmetrics	= opentelemetry::sdk::metrics;
namespace common		= opentelemetry::common;
namespace metricsapi	= opentelemetry::metrics;
#endif

