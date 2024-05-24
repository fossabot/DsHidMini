﻿#include <Shlwapi.h>
#include "GlobalState.h"
#include "UniUtil.h"
#include "DsHidMini/dshmguid.h"


//
// Invoked on device arrival or removal
// 
DWORD CALLBACK GlobalState::DeviceNotificationCallback(
	_In_ HCMNOTIFICATION hNotify,
	_In_opt_ PVOID Context,
	_In_ CM_NOTIFY_ACTION Action,
	_In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
	_In_ DWORD EventDataSize
)
{
	UNREFERENCED_PARAMETER(hNotify);
	UNREFERENCED_PARAMETER(EventDataSize);

	const auto _this = static_cast<GlobalState*>(Context);

	if (_this == nullptr)
	{
		LOG_ERROR("Missing state pointer");
		return ERROR_INVALID_PARAMETER;
	}

	auto scopedSpan = TRACE_SCOPED_SPAN("");

	switch (Action)
	{
	case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
		if (IsEqualGUID(GUID_DEVINTERFACE_DSHIDMINI, EventData->u.DeviceInterface.ClassGuid))
		{
			const auto symlink = std::wstring(EventData->u.DeviceInterface.SymbolicLink);
			LOG_INFO("New DS3 device arrived: {}", ConvertWideToANSI(symlink));

			// child device boot is not instant so we need to do 
			// this in the background to not block this callback
			std::thread asyncArrival{
				[_this, symlink]
				{
					AcquireSRWLockExclusive(&_this->StatesLock);
					{
						DWORD slotIndex = 0;
						if (const auto slot = _this->GetNextFreeSlot(&slotIndex))
						{
							slot->Dispose();
							if (!slot->InitializeAsDs3(symlink))
							{
								LOG_ERROR("Failed to initialize {} as a DS3 HID device", ConvertWideToANSI(symlink));
							}
							else
							{
								LOG_INFO("Assigned {} to index {}", ConvertWideToANSI(symlink), slotIndex);
							}
						}
					}
					ReleaseSRWLockExclusive(&_this->StatesLock);
				}
			};

			asyncArrival.detach();
		}

		if (IsEqualGUID(XUSB_INTERFACE_CLASS_GUID, EventData->u.DeviceInterface.ClassGuid))
		{
			const auto symlink = ConvertWideToANSI(EventData->u.DeviceInterface.SymbolicLink);

			LOG_INFO("New XUSB device arrived: {}", symlink);

			DWORD userIndex = INVALID_X_INPUT_USER_ID;
			if (SymlinkToUserIndex(EventData->u.DeviceInterface.SymbolicLink, &userIndex))
			{
				LOG_INFO("User index: {}", userIndex);

				AcquireSRWLockExclusive(&_this->StatesLock);
				{
					DWORD slotIndex = 0;
					if (const auto slot = _this->GetNextFreeSlot(&slotIndex))
					{
						slot->Dispose();
						if (!slot->InitializeAsXusb(EventData->u.DeviceInterface.SymbolicLink, userIndex))
						{
							LOG_ERROR("Failed to initialize {} as a XUSB device", symlink);
						}
						else
						{
							LOG_INFO("Assigned {} to index {}", symlink, slotIndex);
						}
					}
				}
				ReleaseSRWLockExclusive(&_this->StatesLock);
			}
			else
			{
				LOG_ERROR("User index lookup failed");
			}
		}
		break;
	case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
		if (IsEqualGUID(GUID_DEVINTERFACE_DSHIDMINI, EventData->u.DeviceInterface.ClassGuid))
		{
			const std::string symlink = ConvertWideToANSI(EventData->u.DeviceInterface.SymbolicLink);
			LOG_INFO("DS3 device got removed: {}", symlink);

			AcquireSRWLockExclusive(&_this->StatesLock);
			{
				if (const auto slot = _this->FindBySymbolicLink(EventData->u.DeviceInterface.SymbolicLink))
				{
					slot->Dispose();
				}
				else
				{
					LOG_WARN("No state found for {}", symlink);
				}
			}
			ReleaseSRWLockExclusive(&_this->StatesLock);
		}

		if (IsEqualGUID(XUSB_INTERFACE_CLASS_GUID, EventData->u.DeviceInterface.ClassGuid))
		{
			const std::string symlink = ConvertWideToANSI(EventData->u.DeviceInterface.SymbolicLink);
			LOG_INFO("XUSB device got removed: {}", symlink);

			AcquireSRWLockExclusive(&_this->StatesLock);
			{
				if (const auto slot = _this->FindBySymbolicLink(EventData->u.DeviceInterface.SymbolicLink))
				{
					slot->Dispose();
				}
				else
				{
					LOG_WARN("No state found for {}", symlink);
				}
			}
			ReleaseSRWLockExclusive(&_this->StatesLock);
		}
		break;
	default:
		return ERROR_SUCCESS;
	}

	return ERROR_SUCCESS;
}

//
// Initialization tasks on a background thread
// 
DWORD WINAPI GlobalState::InitAsync(_In_ LPVOID lpParameter)
{
#if defined(SCPLIB_ENABLE_TELEMETRY)
	//
	// Set up tracing
	// 

	const auto resourceAttributes = sdkresource::ResourceAttributes{
		{ opentelemetry::sdk::resource::SemanticConventions::kServiceName, TRACER_NAME }
	};

	const auto resource = sdkresource::Resource::Create(resourceAttributes);
	auto traceExporter = otlp::OtlpGrpcExporterFactory::Create();
	auto traceProcessor = sdktrace::SimpleSpanProcessorFactory::Create(std::move(traceExporter));
	const std::shared_ptr traceProvider = sdktrace::TracerProviderFactory::Create(std::move(traceProcessor), resource);

	trace::Provider::SetTracerProvider(traceProvider);

	//
	// Set up logger
	// 

	auto loggerExporter = otlp::OtlpGrpcLogRecordExporterFactory::Create();
	auto loggerProcessor = sdklogs::SimpleLogRecordProcessorFactory::Create(std::move(loggerExporter));
	const std::shared_ptr loggerProvider = sdklogs::LoggerProviderFactory::Create(std::move(loggerProcessor), resource);

	logs::Provider::SetLoggerProvider(loggerProvider);

	LOG_INFO("Library got loaded into PID {}", GetCurrentProcessId());

	//
	// Set up metrics
	// 

	auto metricsExporter = otlp::OtlpGrpcMetricExporterFactory::Create();

	std::string version{ "1.2.0" };
	std::string schema{ "https://opentelemetry.io/schemas/1.2.0" };

	// Initialize and set the global MeterProvider
	sdkmetrics::PeriodicExportingMetricReaderOptions options;
	options.export_interval_millis = std::chrono::milliseconds(1000);
	options.export_timeout_millis = std::chrono::milliseconds(500);

	auto reader =
	sdkmetrics::PeriodicExportingMetricReaderFactory::Create(std::move(metricsExporter), options);

	auto u_provider = sdkmetrics::MeterProviderFactory::Create();
	auto* p = static_cast<sdkmetrics::MeterProvider*>(u_provider.get());

	p->AddMetricReader(std::move(reader));

	// TODO: implement counters etc.

#endif

	const auto _this = static_cast<GlobalState*>(lpParameter);
	auto scopedSpan = TRACE_SCOPED_SPAN("");

	LOG_INFO("Async library startup initialized");

	CHAR systemDir[MAX_PATH] = {};

	if (GetSystemDirectoryA(systemDir, MAX_PATH) == 0)
	{
		LOG_ERROR("GetSystemDirectoryA failed: {:#x}", GetLastError());
		return GetLastError();
	}

	CHAR fullXiPath[MAX_PATH] = {};

	if (PathCombineA(fullXiPath, systemDir, XI_SYSTEM_LIB_NAME) == nullptr)
	{
		LOG_ERROR("PathCombineA failed: {:#x}", GetLastError());
		return GetLastError();
	}

	const HMODULE xiLib = LoadLibraryA(fullXiPath);

	if (xiLib == nullptr)
	{
		LOG_ERROR("LoadLibraryA failed: {:#x}", GetLastError());
		return GetLastError();
	}

	//
	// Grab the function pointers from the OS-provided exports
	// 

	_this->FpnXInputGetState = reinterpret_cast<decltype(XInputGetState)*>(GetProcAddress(xiLib,
		NAMEOF(XInputGetState)
	));
	_this->FpnXInputSetState = reinterpret_cast<decltype(XInputSetState)*>(GetProcAddress(xiLib,
		NAMEOF(XInputSetState)
	));
	_this->FpnXInputGetCapabilities = reinterpret_cast<decltype(XInputGetCapabilities)*>(GetProcAddress(xiLib,
		NAMEOF(XInputGetCapabilities)
	));
	_this->FpnXInputEnable = reinterpret_cast<decltype(XInputEnable)*>(GetProcAddress(xiLib,
		NAMEOF(XInputEnable)
	));
	_this->FpnXInputGetDSoundAudioDeviceGuids = reinterpret_cast<decltype(XInputGetDSoundAudioDeviceGuids)*>(GetProcAddress(xiLib,
		NAMEOF(XInputGetDSoundAudioDeviceGuids)
	));
	_this->FpnXInputGetBatteryInformation = reinterpret_cast<decltype(XInputGetBatteryInformation)*>(GetProcAddress(xiLib,
		NAMEOF(XInputGetBatteryInformation)
	));
	_this->FpnXInputGetKeystroke = reinterpret_cast<decltype(XInputGetKeystroke)*>(GetProcAddress(xiLib,
		NAMEOF(XInputGetKeystroke)
	));
	_this->FpnXInputGetStateEx = reinterpret_cast<decltype(XInputGetStateEx)*>(GetProcAddress(xiLib,
		MAKEINTRESOURCEA(100)
	));
	_this->FpnXInputWaitForGuideButton = reinterpret_cast<decltype(XInputWaitForGuideButton)*>(GetProcAddress(xiLib,
		MAKEINTRESOURCEA(101)
	));
	_this->FpnXInputCancelGuideButtonWait = reinterpret_cast<decltype(XInputCancelGuideButtonWait)*>(GetProcAddress(xiLib,
		MAKEINTRESOURCEA(102)
	));
	_this->FpnXInputPowerOffController = reinterpret_cast<decltype(XInputPowerOffController)*>(GetProcAddress(xiLib,
		MAKEINTRESOURCEA(103)
	));

	//
	// Register notifications for device arrival/removal
	// 

	CM_NOTIFY_FILTER ds3Filter = {};
	ds3Filter.cbSize = sizeof(CM_NOTIFY_FILTER);
	ds3Filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
	ds3Filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_DSHIDMINI;

	//
	// Register DsHidMini device interface
	// 
	CONFIGRET ret = CM_Register_Notification(&ds3Filter, _this, DeviceNotificationCallback, &_this->Ds3NotificationHandle);

	if (ret != CR_SUCCESS)
	{
		LOG_ERROR("CM_Register_Notification (DS3) failed: {:#x}", ret);
	}

	CM_NOTIFY_FILTER xusbFilter = {};
	xusbFilter.cbSize = sizeof(CM_NOTIFY_FILTER);
	xusbFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
	xusbFilter.u.DeviceInterface.ClassGuid = XUSB_INTERFACE_CLASS_GUID;

	//
	// Register X360/XBONE device interface
	// 
	ret = CM_Register_Notification(&xusbFilter, _this, DeviceNotificationCallback, &_this->XusbNotificationHandle);

	if (ret != CR_SUCCESS)
	{
		LOG_ERROR("CM_Register_Notification (XUSB) failed: {:#x}", ret);
	}

	if (const auto result = hid_init(); result != 0)
	{
		LOG_ERROR("hid_init failed: {}", ConvertWideToANSI(hid_error(nullptr)));
	}

	_this->EnumerateDs3Devices();
	_this->EnumerateXusbDevices();

	SetEvent(_this->StartupFinishedEvent);

	return ERROR_SUCCESS;
}
