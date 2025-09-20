#include <windows.h>

#include <cstdint>
#include <evntcons.h>
#include <evntrace.h>
#include <stdio.h>
#include <tdh.h>
#include <wchar.h>

#pragma comment(lib, "tdh.lib")


// The process and data is taken from this article
// https://helgeklein.com/blog/how-to-check-which-bluetooth-a2dp-audio-codec-is-used-on-windows/

struct CodecData {
  const uint8_t A2dpStandardCodecId;
  const uint32_t A2dpVendorId;
  const uint16_t A2dpVendorCodecId;
  const WCHAR *Name;
};

const CodecData CODECS[] = {
    {0, 0, 0, L"SBC"},
    {0x01, 0, 0, L"MPEG-1,2 (aka MP3)"},
    {0x02, 0, 0, L"MPEG-2,4 (aka AAC)"},
    {0x03, 0, 0, L"ATRAC"},
    {0xFF, 0x004F, 0x0, L"aptX"},
    {0xFF, 0x00D7, 0x24, L"aptX HD"},
    {0xFF, 0x000A, 0x02, L"aptX Low Latency"},
    {0xFF, 0x00D7, 0x02, L"aptX Low Latency"},
    {0xFF, 0x000A, 0x01, L"FastStream"},
    {0xFF, 0x012D, 0xAA, L"LDAC"},
    {0xFF, 0x0075, 0x0102, L"Samsung HD"},
    {0xFF, 0x0075, 0x0103, L"Samsung Scalable Codec"},
    {0xFF, 0x053A, 0x484C, L"Savitech LHDC"},
    {0xFF, 0x000A, 0x0104, L"The CSR True Wireless Stereo v3 Codec ID for AAC"},
    {0xFF, 0x000A, 0x0105, L"The CSR True Wireless Stereo v3 Codec ID for MP3"},
    {0xFF, 0x000A, 0x0106,
     L"The CSR True Wireless Stereo v3 Codec ID for aptX"}};

// The GUID for the provider we want to trace
static GUID ProviderGuid = {0x8776ad1e,
                            0x5022,
                            0x4451,
                            {0xa5, 0x66, 0xf4, 0x7e, 0x70, 0x8b, 0x90, 0x75}};
// The name of the ETW session
const WCHAR *sessionName = L"BT_CODEC";

// Global variables to hold handles and properties for the ETW session
// These are global so they can be accessed by the Ctrl+C handler
TRACEHANDLE hSession = 0;
TRACEHANDLE hTrace = 0;
EVENT_TRACE_PROPERTIES *pSessionProperties = NULL;

// Cleans up resources used by the ETW session
void Cleanup() {
  // Stop the trace session if it was started
  if (hSession != 0) {
    ControlTraceW(hSession, NULL, pSessionProperties, EVENT_TRACE_CONTROL_STOP);
  }
  // Close the trace handle if it was opened
  if (hTrace != 0) {
    CloseTrace(hTrace);
  }
  // Free the memory allocated for the session properties
  if (pSessionProperties != NULL) {
    free(pSessionProperties);
  }
}

// Handles the Ctrl+C signal to gracefully stop the trace
BOOL WINAPI CtrlCHandler(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
  case CTRL_C_EVENT:
    wprintf(L"Ctrl+C received, stopping trace...\n");
    Cleanup();
    return TRUE;
  default:
    return FALSE;
  }
}

// Callback function that processes each event received from the ETW session
void WINAPI ProcessEvent(PEVENT_RECORD pEvent) {
  DWORD status = ERROR_SUCCESS;
  PTRACE_EVENT_INFO pInfo = NULL;
  DWORD bufferSize = 0;

  // Get the size of the buffer required for the event information
  status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);
  if (ERROR_INSUFFICIENT_BUFFER == status) {
    // Allocate memory for the event information
    pInfo = (PTRACE_EVENT_INFO)malloc(bufferSize);
    if (pInfo == NULL) {
      wprintf(L"Failed to allocate memory for event info\n");
      return;
    }

    // Get the event information
    status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);
  }

  if (ERROR_SUCCESS != status) {
    wprintf(L"TdhGetEventInformation failed with %lu\n", status);
    if (pInfo) {
      free(pInfo);
    }
    return;
  }

  // Print general event information
  wprintf(L"Event ID: %u\n", pEvent->EventHeader.EventDescriptor.Id);
  wprintf(L"Provider GUID: ");
  for (int i = 0; i < 8; i++) {
  //wprintf(L"%02x", pEvent->EventHeader.ProviderId.Data4[i]);
  }
  wprintf(L"\n");

  // Sizes of the data
  // Property size: 1
  // A2dpStandardCodecId: 02
  // Property size: 4
  // A2dpVendorId: 00000000
  // Property size: 2
  // A2dpVendorCodecId: 0000
  int8_t a2dpStandardCodecId = -1;
  int32_t a2dpVendorId = -1;
  int16_t a2dpVendorCodecId = -1;

  // Iterate through the properties of the event
  for (ULONG i = 0; i < pInfo->TopLevelPropertyCount; i++) {
    PROPERTY_DATA_DESCRIPTOR descriptor;
    descriptor.PropertyName =
        (ULONGLONG)((PBYTE)pInfo + pInfo->EventPropertyInfoArray[i].NameOffset);
    descriptor.ArrayIndex = 0;

    // Get the size of the property data
    ULONG propertySize = 0;
    status = TdhGetPropertySize(pEvent, 0, NULL, 1, &descriptor, &propertySize);
    if (status != ERROR_SUCCESS) {
      continue;
    }

    // Allocate memory for the property data
    PBYTE pPropertyData = (PBYTE)malloc(propertySize);
    if (pPropertyData == NULL) {
      continue;
    }

    // Get the property data
    status = TdhGetProperty(pEvent, 0, NULL, 1, &descriptor, propertySize,
                            pPropertyData);
    if (status == ERROR_SUCCESS) {
      if (wcscmp((PWSTR)descriptor.PropertyName, L"A2dpStandardCodecId") == 0) {
        a2dpStandardCodecId = *(int32_t *)pPropertyData;
      } else if (wcscmp((PWSTR)descriptor.PropertyName, L"A2dpVendorId") == 0) {
        a2dpVendorId = *(int32_t *)pPropertyData;
      } else if (wcscmp((PWSTR)descriptor.PropertyName, L"A2dpVendorCodecId") ==
                 0) {
        a2dpVendorCodecId = *(int16_t *)pPropertyData;
      }

      //wprintf(L"Property size: %d\n", propertySize);
      wprintf(L"  %s: ", (PWSTR)descriptor.PropertyName);
      // If the property is AvdtpActivity or ResultCode, print its integer
      // value
      if (wcscmp((PWSTR)descriptor.PropertyName, L"AvdtpActivity") == 0 ||
          wcscmp((PWSTR)descriptor.PropertyName, L"ResultCode") == 0) {
        wprintf(L"%d", *(int *)pPropertyData);
      } else {
        // Otherwise, print the raw hex bytes
        for (ULONG k = 0; k < propertySize; k++) {
          wprintf(L"%02x", pPropertyData[k]);
        }
      }
      wprintf(L"\n");
    }
    free(pPropertyData);
  }


  // Find coded if codec data was specified
  if (a2dpStandardCodecId != -1) {
    //wprintf(L"Look for codec with %d, %d, %d\n", a2dpStandardCodecId,
            //a2dpVendorId, a2dpStandardCodecId);
    for (const auto &codec : CODECS) {
      // if a set of a2dpStandardCodecId is in 0,1,2 or 4 then
      // we should not check other fields
      if (a2dpStandardCodecId == 0x00 || a2dpStandardCodecId == 0x01 ||
          a2dpStandardCodecId == 0x02 || a2dpStandardCodecId == 0x04) {
        if (codec.A2dpStandardCodecId == a2dpStandardCodecId) {
          wprintf(L"-------------------------------------\n");
          wprintf(L">  Codec Name: %s\n", codec.Name);
          wprintf(L"-------------------------------------\n");
        }
      } else if (codec.A2dpStandardCodecId == a2dpStandardCodecId &&
                 codec.A2dpVendorId == a2dpVendorId &&
                 codec.A2dpVendorCodecId == a2dpVendorCodecId) {
        wprintf(L"      Codec Name: %s\n", codec.Name);
        break;
      }
    }
  }

  if (pInfo) {
    free(pInfo);
  }
}

int main() {
  // Allocate memory for the session properties
  ULONG bufferSize = sizeof(EVENT_TRACE_PROPERTIES) +
                     (wcslen(sessionName) + 1) * sizeof(WCHAR);
  pSessionProperties = (EVENT_TRACE_PROPERTIES *)malloc(bufferSize);
  if (pSessionProperties == NULL) {
    wprintf(L"Unable to allocate memory for properties\n");
    return 1;
  }

  // Zero out the memory and set the session properties
  ZeroMemory(pSessionProperties, bufferSize);
  pSessionProperties->Wnode.BufferSize = bufferSize;
  pSessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
  pSessionProperties->Wnode.ClientContext = 1; // Use QPC for timestamp
  pSessionProperties->Wnode.Guid = ProviderGuid;
  pSessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
  pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

  // Start the trace session
  ULONG status = StartTraceW(&hSession, sessionName, pSessionProperties);
  // If the session already exists, attach to it
  if (status == ERROR_ALREADY_EXISTS) {
    wprintf(L"Attaching to existing session...\n");
    // Query the existing session to get its handle
    status = ControlTraceW(0, sessionName, pSessionProperties,
                           EVENT_TRACE_CONTROL_QUERY);
    if (status != ERROR_SUCCESS) {
      wprintf(L"ControlTrace failed with %lu\n", status);
      Cleanup();
      return 1;
    }
    hSession = pSessionProperties->Wnode.HistoricalContext;
  } else if (status != ERROR_SUCCESS) {
    wprintf(L"StartTrace failed with %lu\n", status);
    Cleanup();
    return 1;
  }

  // Enable the provider for the session
  status = EnableTraceEx2(hSession, &ProviderGuid,
                          EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                          TRACE_LEVEL_VERBOSE, 0, 0, 0, NULL);
  if (status != ERROR_SUCCESS) {
    wprintf(L"EnableTraceEx2 failed with %lu\n", status);
    Cleanup();
    return 1;
  }

  // Set the Ctrl+C handler
  if (!SetConsoleCtrlHandler(CtrlCHandler, TRUE)) {
    wprintf(L"SetConsoleCtrlHandler failed with %lu\n", GetLastError());
    Cleanup();
    return 1;
  }

  // Set up the log file structure to process events in real time
  EVENT_TRACE_LOGFILEW logFile = {0};
  logFile.LoggerName = (LPWSTR)sessionName;
  logFile.ProcessTraceMode =
      PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
  logFile.EventRecordCallback = ProcessEvent;

  // Open the trace to start processing events
  hTrace = OpenTraceW(&logFile);
  if (INVALID_PROCESSTRACE_HANDLE == hTrace) {
    wprintf(L"OpenTrace failed with %lu\n", GetLastError());
    Cleanup();
    return 1;
  }

  wprintf(L"Listening for events... Press Ctrl+C to stop.\n");
  // Start processing traces
  ProcessTrace(&hTrace, 1, 0, 0);

  // Clean up resources before exiting
  Cleanup();

  return 0;
}
