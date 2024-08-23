#include "Ring0EventSender.h"
#include <fltKernel.h>
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"

#define PIPE_NAME L"\\Device\\NamedPipe\\NamedPipeMaster"

PFLT_FILTER gFilterHandle = NULL;
PFLT_PORT gFltPort = NULL;
PFLT_PORT gClientPort = NULL;

NTSTATUS SendEvent(PVOID data, ULONG length)
{
    // return NamedPipeSendData(data, length);
    return MinifltPortSendData(data, length);
}

NTSTATUS NamedPipeSendData(PVOID data, ULONG length)
{
    UNREFERENCED_PARAMETER(data);
    UNICODE_STRING pipeName;
    RtlInitUnicodeString(&pipeName, PIPE_NAME);

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &pipeName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS status = STATUS_SUCCESS;
    HANDLE pipeHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    status = ZwOpenFile(&pipeHandle, FILE_GENERIC_WRITE, &objAttr, &ioStatusBlock, FILE_SHARE_WRITE, 0);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("ZwOpenFile failed: %08X\n", status);
        goto _EXIT;
    }

    status = ZwWriteFile(pipeHandle, NULL, NULL, NULL, &ioStatusBlock, &length, sizeof(length), NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("ZwWriteFile failed: %08X\n", status);
        goto _EXIT;
    }

    status = ZwWriteFile(pipeHandle, NULL, NULL, NULL, &ioStatusBlock, data, length, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("ZwWriteFile failed: %08X\n", status);
        goto _EXIT;
    }

_EXIT:
    if (pipeHandle)
    {
        ZwClose(pipeHandle);
    }

    return status;
}

NTSTATUS MinifltPortInitialize(_In_ PFLT_FILTER filterHandle)
{
    NTSTATUS status = STATUS_SUCCESS;

    PSECURITY_DESCRIPTOR sd = {0};
    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("FltBuildDefaultSecurityDescriptor error: %X\n", status);
        return status;
    }

    UNICODE_STRING portName = {0};
    RtlInitUnicodeString(&portName, NAMED_PIPE_MASTER_PORT_NAME);
    OBJECT_ATTRIBUTES oa = {0};
    InitializeObjectAttributes(&oa, &portName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);

    status = FltCreateCommunicationPort(filterHandle, &gFltPort, &oa, NULL, (PFLT_CONNECT_NOTIFY)MinifltPortNotifyRoutine,
                                        (PFLT_DISCONNECT_NOTIFY)MinifltPortDisconnectRoutine, NULL, 1);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("FltCreateCommunicationPort error: %X\n", status);
        MinifltPortFinalize();
        return status;
    }

    gFilterHandle = filterHandle;

    return status;
}

VOID MinifltPortFinalize()
{
    if (gFltPort)
    {
        FltCloseCommunicationPort(gFltPort);
    }
}


NTSTATUS MinifltPortNotifyRoutine(_In_ PFLT_PORT connectedClientPort, _In_ PVOID serverCookie, _In_ PVOID connectionContext,
                                  _In_ ULONG connectionContextSize, _Out_ PVOID* connectionPortCookie)
{
    UNREFERENCED_PARAMETER(serverCookie);
    UNREFERENCED_PARAMETER(connectionContext);
    UNREFERENCED_PARAMETER(connectionContextSize);
    UNREFERENCED_PARAMETER(connectionPortCookie);

    DbgPrint("[filterport] " __FUNCTION__ " User-mode application(%u) connect to this filter\n",
             HandleToUlong(PsGetCurrentProcessId()));

    gClientPort = connectedClientPort;

    return STATUS_SUCCESS;
}

VOID MinifltPortDisconnectRoutine(_In_ PVOID connectionCookie)
{
    UNREFERENCED_PARAMETER(connectionCookie);

    DbgPrint("[filterport] " __FUNCTION__ " User-mode application(%u) disconnect with this filter\n",
             HandleToUlong(PsGetCurrentProcessId()));
}

NTSTATUS MinifltPortSendData(_In_ PVOID data, _In_ ULONG length)
{
    NTSTATUS status = FltSendMessage(gFilterHandle, &gClientPort, data, length, NULL, NULL, NULL);
    return status;
}
