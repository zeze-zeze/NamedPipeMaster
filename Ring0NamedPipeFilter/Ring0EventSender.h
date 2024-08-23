#pragma once
#include <fltKernel.h>

NTSTATUS SendEvent(PVOID data, ULONG length);
NTSTATUS NamedPipeSendData(PVOID data, ULONG length);
NTSTATUS MinifltPortInitialize(_In_ PFLT_FILTER gFilterHandle);

VOID MinifltPortFinalize();
NTSTATUS MinifltPortNotifyRoutine(_In_ PFLT_PORT client_port, _In_ PVOID server_cookie, _In_ PVOID connection_context,
                                  _In_ ULONG connection_context_size, _Out_ PVOID* connection_port_cookie);
VOID MinifltPortDisconnectRoutine(_In_ PVOID connection_cookie);
NTSTATUS MinifltPortMessageRoutine(_In_ PVOID port_cookie, _In_opt_ PVOID input_buffer, _In_ ULONG input_buffer_size,
                                   _Out_opt_ PVOID output_buffer, _In_ ULONG output_buffer_size,
                                   _Out_ PULONG return_output_buffer_length);
NTSTATUS MinifltPortSendData(_In_ PVOID data, _In_ ULONG length);
