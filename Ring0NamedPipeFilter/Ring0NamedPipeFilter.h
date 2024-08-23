#pragma once

#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"

extern "C"
{
    struct NPM_FILE_NAME_INFO
    {
        ULONG fileNameLength;
        WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
    };

    DRIVER_INITIALIZE DriverEntry;

    NTSTATUS FLTAPI NPMfInstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
                                      _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);

    NTSTATUS FLTAPI NPMfInstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                                              _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

    NTSTATUS FLTAPI NPMfUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

    FLT_PREOP_CALLBACK_STATUS FLTAPI NPMfPreCreateNamedPipe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _Flt_CompletionContext_Outptr_ PVOID *CompletionContext);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostCreateNamedPipe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                              _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                              _In_opt_ PVOID CompletionContext,
                                                              _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostRead(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                   _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostReadWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                           _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                           _In_opt_ PVOID CompletionContext,
                                                           _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostWrite(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                    _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostWriteWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _In_opt_ PVOID CompletionContext,
                                                            _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostFSCtl(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                    _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags);

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostFSCtlWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _In_opt_ PVOID CompletionContext,
                                                            _In_ FLT_POST_OPERATION_FLAGS Flags);

}    // extern "C"
