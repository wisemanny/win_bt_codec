// MIT License
//
// Copyright (c) 2025 The win_bt_codec project authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cwchar>
#include <windows.h>

#include <evntcons.h>
#include <evntrace.h>
#include <stdio.h>
#include <tdh.h>
#include <wchar.h>

#include <optional>

// The process and data is taken from this article
// https://helgeklein.com/blog/how-to-check-which-bluetooth-a2dp-audio-codec-is-used-on-windows/

// Set to true to print events data
bool TRACE_EVENTS = false;

struct CodecData {
    const BYTE A2dpStandardCodecId;
    const DWORD A2dpVendorId;
    const WORD A2dpVendorCodecId;
    const WCHAR *Name;
};

const CodecData CODECS[] = {
    {0x0, 0x0, 0x0, L"SBC"},
    {0x01, 0x0, 0x0, L"MPEG-1,2 (aka MP3)"},
    {0x02, 0x0, 0x0, L"MPEG-2,4 (aka AAC)"},
    {0x03, 0x0, 0x0, L"ATRAC"},
    {0xFF, 0x004F, 0x01, L"aptX"},
    {0xFF, 0x00D7, 0x24, L"aptX HD"},
    {0xFF, 0x000A, 0x02, L"aptX Low Latency"},
    {0xFF, 0x00D7, 0x02, L"aptX Low Latency"},
    {0xFF, 0x000A, 0x01, L"FastStream"},
    {0xFF, 0x012D, 0xAA, L"LDAC"},
    {0xFF, 0x0075, 0x0102, L"Samsung HD"},
    {0xFF, 0x0075, 0x0103, L"Samsung Scalable Codec"},
    {0xFF, 0x053A, 0x484C, L"Savitech LHDC"},
    {0xFF, 0x000A, 0x0103, L"Qualcomm specific aptX version"},
    {0xFF, 0x000A, 0x0104, L"The CSR True Wireless Stereo v3 Codec ID for AAC"},
    {0xFF, 0x000A, 0x0105, L"The CSR True Wireless Stereo v3 Codec ID for MP3"},
    {0xFF, 0x000A, 0x0106,
     L"The CSR True Wireless Stereo v3 Codec ID for aptX"}};

struct A2dpEventData {
    std::optional<BYTE> a2dpStandardCodecId;
    std::optional<DWORD> a2dpVendorId;
    std::optional<WORD> a2dpVendorCodecId;
    std::optional<DWORD> AvdtpActivity;
    std::optional<WORD> ResultCode;
    std::optional<DWORD> SampleRate;
    std::optional<DWORD> ChannelCount;
    std::optional<BYTE> AcceptorStreamEndPointID;
    std::optional<BYTE> InitiatorStreamEndPointID;
};

// These vaklues were reversed from btha2dp.sys on Windows 11 build 26100
// and I am not sure if this going to stay the same in new builds.
// I explained the process in the events.md file
enum AvdtpActivity {
    Abort_Cfm = 0x2e,
    Abort_Ind = 0x2f,
    Close_Cfm = 0x26,
    Close_Ind = 0x27,
    Connect_Cfm = 0x2,
    Connect_Ind = 0x3,
    Disconnect_Cfm = 0x6,
    Disconnect_Ind = 0x7,
    Discover_Cfm = 10,
    Discover_Ind = 0xb,
    GetCapabilities_Cfm = 0xe,
    GetCapabilities_ind = 0xf,
    GetConfiguration_Cfm = 0x16,
    GetConfiguration_Ind = 0x17,
    SetConfiguration_Cfm = 0x12,
    SetConfiguration_Ind_1 = 0x13, // in the same func it goes 0x13 and then
                                   // 0x14
    SetConfiguration_Ind_2 = 0x14,

    Open_Cfm = 0x1e,
    Open_Ind = 0x1f,
    Reconfigure_Cfm = 0x1a,
    Reconfigure_Ind_1 = 0x1c, // Same as above, it goes 0x1c and then 0x1b
    Reconfigure_Ind_2 = 0x1b,
    Start_Cfm = 0x22,
    Start_Ind = 0x24,
    Suspend_Cfm = 0x2a,
    Suspend_Ind = 0x2c,

    AbortStream = 0x2d,
    EnterNewStreamingState = 0x29,
    FindNextSepAndGepCaps = 0xd,
    RequestOpenMediaChannel = 0x1d,
    RequestStreamClose = 0x25,
    RequestStreamOpen = 9,

    SendReconfigurationRequest = 0x19,
};

const WCHAR *AvdtpActivityToString(AvdtpActivity activity) {
    switch (activity) {
    case Abort_Cfm:
        return L"Abort_Cfm";
    case Abort_Ind:
        return L"Abort_Ind";
    case Close_Cfm:
        return L"Close_Cfm";
    case Close_Ind:
        return L"Close_Ind";
    case Connect_Cfm:
        return L"Connect_Cfm";
    case Connect_Ind:
        return L"Connect_Ind";
    case Disconnect_Cfm:
        return L"Disconnect_Cfm";
    case Disconnect_Ind:
        return L"Disconnect_Ind";
    case Discover_Cfm:
        return L"Discover_Cfm";
    case Discover_Ind:
        return L"Discover_Ind";
    case GetCapabilities_Cfm:
        return L"GetCapabilities_Cfm";
    case GetCapabilities_ind:
        return L"GetCapabilities_ind";
    case GetConfiguration_Cfm:
        return L"GetConfiguration_Cfm";
    case GetConfiguration_Ind:
        return L"GetConfiguration_Ind";
    case SetConfiguration_Cfm:
        return L"SetConfiguration_Cfm";
    case SetConfiguration_Ind_1:
        return L"SetConfiguration_Ind_1";
    case SetConfiguration_Ind_2:
        return L"SetConfiguration_Ind_2";
    case Reconfigure_Ind_1:
        return L"Reconfigure_Ind_1";
    case Reconfigure_Ind_2:
        return L"Reconfigure_Ind_2";
    case Open_Cfm:
        return L"Open_Cfm";
    case Open_Ind:
        return L"Open_Ind";
    case Reconfigure_Cfm:
        return L"Reconfigure_Cfm";
    case Start_Cfm:
        return L"Start_Cfm";
    case Start_Ind:
        return L"Start_Ind";
    case Suspend_Cfm:
        return L"Suspend_Cfm";
    case Suspend_Ind:
        return L"Suspend_Ind";
    case AbortStream:
        return L"AbortStream";
    case EnterNewStreamingState:
        return L"EnterNewStreamingState";
    case FindNextSepAndGepCaps:
        return L"FindNextSepAndGepCaps";
    case RequestOpenMediaChannel:
        return L"RequestOpenMediaChannel";
    case RequestStreamClose:
        return L"RequestStreamClose";
    case RequestStreamOpen:
        return L"RequestStreamOpen";
    case SendReconfigurationRequest:
        return L"SendReconfigurationRequest";
    default:
        return L"Unknown";
    }
}

// The GUID for the provider we want to trace
// This one is Microsoft.Windows.Bluetooth.BthA2dp
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
        ControlTraceW(hSession, NULL, pSessionProperties,
                      EVENT_TRACE_CONTROL_STOP);
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

void TraceEventInfo(const PEVENT_RECORD pEvent, const PTRACE_EVENT_INFO pInfo) {
    DWORD status = ERROR_SUCCESS;

    // Print general event information
    wprintf(L"--------------------------------------\n");
    wprintf(L"Event ID: %u\n", pEvent->EventHeader.EventDescriptor.Id);
    //wprintf(L"Provider GUID: ");
    //for (int i = 0; i < 8; i++) {
        // wprintf(L"%02x", pEvent->EventHeader.ProviderId.Data4[i]);
    //}
    //wprintf(L"\n");

    // Iterate through the properties of the event
    for (ULONG i = 0; i < pInfo->TopLevelPropertyCount; i++) {
        PROPERTY_DATA_DESCRIPTOR descriptor;
        descriptor.PropertyName =
            (ULONGLONG)((PBYTE)pInfo +
                        pInfo->EventPropertyInfoArray[i].NameOffset);
        descriptor.ArrayIndex = 0;

        // Get the size of the property data
        ULONG propertySize = 0;
        status =
            TdhGetPropertySize(pEvent, 0, NULL, 1, &descriptor, &propertySize);
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
            // wprintf(L"Property size: %d\n", propertySize);
            wprintf(L"  %s [%d]: ", (PWSTR)descriptor.PropertyName,
                    propertySize);
            // Otherwise, print the raw hex bytes
            for (ULONG k = 0; k < propertySize; k++) {
                wprintf(L"%02x", pPropertyData[k]);
            }

            if (wcscmp((PWSTR)descriptor.PropertyName, L"AvdtpActivity") == 0) {
                AvdtpActivity activityValue =
                    static_cast<AvdtpActivity>(*(DWORD *)pPropertyData);
                auto activity = AvdtpActivityToString(activityValue);
                wprintf(L"     [%s]", activity);
            }

            wprintf(L"\n");
            free(pPropertyData);
        }
    }
}

// Returns pointer to codec name or nullptr if codec was not found
const WCHAR *GetCodecName(const A2dpEventData &eventData) {
    // Find coded if codec data was specified
    if (eventData.a2dpStandardCodecId) {
        // wprintf(L"Look for codec with %d, %d, %d\n", a2dpStandardCodecId,
        // a2dpVendorId, a2dpStandardCodecId);
        for (const auto &codec : CODECS) {
            // if a set of a2dpStandardCodecId is in 0,1,2 or 4 then
            // we should not check other fields
            if (eventData.a2dpStandardCodecId == 0x00 ||
                eventData.a2dpStandardCodecId == 0x01 ||
                eventData.a2dpStandardCodecId == 0x02 ||
                eventData.a2dpStandardCodecId == 0x04) {
                if (codec.A2dpStandardCodecId ==
                    eventData.a2dpStandardCodecId) {
                    return codec.Name;
                }
            } else if (codec.A2dpStandardCodecId ==
                           eventData.a2dpStandardCodecId &&
                       codec.A2dpVendorId == eventData.a2dpVendorId &&
                       codec.A2dpVendorCodecId == eventData.a2dpVendorCodecId) {
                return codec.Name;
            }
        }
    }

    return nullptr;
}

// Returns true if all OK or false if error happened during processing
bool ProcessEventData(const A2dpEventData &eventData) {
    // Here is absolute guess based on the events I saw
    // 1. If AcceptorStreamEndPointID is defined but
    // InitiatorStreamEndPointID and codec data is defined, seems like that
    // is receiver transmits its list of supported codecs
    if (eventData.AcceptorStreamEndPointID &&
        !eventData.InitiatorStreamEndPointID && eventData.a2dpStandardCodecId) {
        const WCHAR *codecName = GetCodecName(eventData);
        if (codecName != nullptr) {
            wprintf(L"> Received supported codec: %s\n", codecName);
        } else {
            wprintf(L"ERROR: Unknown codec\n");
            return false;
        }
    }

    // 2. If AcceptorStreamEndPointID and InitiatorStreamEndPointID are
    // defined and if AvdtpActivity is defined and equal to 0x12, seems like
    // that means that codec was selected
    if (eventData.AcceptorStreamEndPointID &&
        eventData.InitiatorStreamEndPointID && eventData.AvdtpActivity &&
        (eventData.AvdtpActivity == AvdtpActivity::SetConfiguration_Cfm ||
         eventData.AvdtpActivity == AvdtpActivity::SetConfiguration_Ind_2)) {
        const WCHAR *codecName = GetCodecName(eventData);
        if (codecName != nullptr) {
            wprintf(L"# Selected codec: %s\n", codecName);
        } else {
            wprintf(L"ERROR: Unknown codec\n");
            return false;
        }
    }

    return true;
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

    if (TRACE_EVENTS) {
        TraceEventInfo(pEvent, pInfo);
    }

    // Sizes of the data
    // Property size: 1
    // A2dpStandardCodecId: 02
    // Property size: 4
    // A2dpVendorId: 00000000
    // Property size: 2
    // A2dpVendorCodecId: 0000
    // Sample Rate: 80bb0000
    // ChannelCount: 02000000
    A2dpEventData eventData;

    // Iterate through the properties of the event
    for (ULONG i = 0; i < pInfo->TopLevelPropertyCount; i++) {
        PROPERTY_DATA_DESCRIPTOR descriptor;
        descriptor.PropertyName =
            (ULONGLONG)((PBYTE)pInfo +
                        pInfo->EventPropertyInfoArray[i].NameOffset);
        descriptor.ArrayIndex = 0;

        // Get the size of the property data
        ULONG propertySize = 0;
        status =
            TdhGetPropertySize(pEvent, 0, NULL, 1, &descriptor, &propertySize);
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
            if (wcscmp((PWSTR)descriptor.PropertyName,
                       L"A2dpStandardCodecId") == 0) {
                eventData.a2dpStandardCodecId = *(BYTE *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"A2dpVendorId") == 0) {
                eventData.a2dpVendorId = *(DWORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"A2dpVendorCodecId") == 0) {
                eventData.a2dpVendorCodecId = *(WORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"AvdtpActivity") == 0) {
                eventData.AvdtpActivity = *(DWORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName, L"ResultCode") ==
                       0) {
                eventData.ResultCode = *(WORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName, L"Sample Rate") ==
                       0) {
                eventData.SampleRate = *(DWORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"ChannelCount") == 0) {
                eventData.ChannelCount = *(DWORD *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"AcceptorStreamEndPointID") == 0) {
                eventData.AcceptorStreamEndPointID = *(BYTE *)pPropertyData;
            } else if (wcscmp((PWSTR)descriptor.PropertyName,
                              L"InitiatorStreamEndPointID") == 0) {
                eventData.InitiatorStreamEndPointID = *(BYTE *)pPropertyData;
            }
        }
        free(pPropertyData);
    }

    if (!ProcessEventData(eventData)) {
        TraceEventInfo(pEvent, pInfo);
    }

    if (pInfo) {
        free(pInfo);
    }
}

int main(int argc, char *argv[]) {

    // Very simple cmd args parsing
    if (argc == 2) {
        const char *param = argv[1];
        if (strcmp(param, "--help") == 0 || strcmp(param, "-h") == 0) {
            wprintf(L"Help:\n");
            wprintf(L"   -h|--help to print help\n");
            wprintf(L"   -t|--trace to print all ETW events\n");
            return 1;
        } else if (strcmp(param, "--trace") == 0 || strcmp(param, "-t") == 0) {
            wprintf(L"Trace: Print all events for tracing purposes\n");
            TRACE_EVENTS = true;
        }
    }

    // Allocate memory for the session properties
    const size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) +
                              (wcslen(sessionName) + 1) * sizeof(WCHAR);
    pSessionProperties = (EVENT_TRACE_PROPERTIES *)malloc(bufferSize);
    if (pSessionProperties == NULL) {
        wprintf(L"Unable to allocate memory for properties\n");
        return 1;
    }

    // Zero out the memory and set the session properties
    ZeroMemory(pSessionProperties, bufferSize);
    pSessionProperties->Wnode.BufferSize = (ULONG)bufferSize;
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
    wprintf(L"Now please connect the Bluetooth device to the computer.\n");
    wprintf(L"Note: this app uses internal Windows messages and analysis is\n"
            L"      based on guess. So results may be not accurate.\n\n");

    // Start processing traces
    ProcessTrace(&hTrace, 1, 0, 0);

    // Clean up resources before exiting
    Cleanup();

    return 0;
}
