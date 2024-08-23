#include "DetourFunction.h"
#include <Windows.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <MinHook.h>
#include "../NamedPipeMasterBase/utils.h"
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"
#include "Ring3EventSender.h"

tNtCreateFile fpNtCreateFile = NULL;
tNtCreateFile ntdllNtCreateFile = NULL;
tNtCreateNamedPipeFile fpNtCreateNamedPipeFile = NULL;
tNtCreateNamedPipeFile ntdllNtCreateNamedPipeFile = NULL;
tNtReadFile fpNtReadFile = NULL;
tNtReadFile ntdllNtReadFile = NULL;
tNtWriteFile fpNtWriteFile = NULL;
tNtWriteFile ntdllNtWriteFile = NULL;
tNtFsControlFile fpNtFsControlFile = NULL;
tNtFsControlFile ntdllNtFsControlFile = NULL;

NTSTATUS WINAPI DetourNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                                   PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
                                   ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
                                   ULONG EaLength)
{
    NTSTATUS ret = fpNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes,
                                  ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

    // Check if it is a named pipe handle.
    if (GetFileType(*FileHandle) != FILE_TYPE_PIPE)
    {
        return ret;
    }

    // Exclude events made by ourselves.
    std::wstring fileName = GetFileNameFromHandle(*FileHandle);
    if (fileName.find(NAMED_PIPE_MASTER) != std::wstring::npos)
    {
        return ret;
    }

    ULONG dataLength = sizeof(DATA_CREATE) + 2;
    DATA_CREATE* data = (DATA_CREATE*)malloc(dataLength);
    if (!data)
    {
        return ret;
    }
    memset(data, 0, dataLength);

    data->cmdId = CMD_CREATE;
    data->sourceType = SOURCE_INJECTED_DLL;
    data->threadId = GetCurrentThreadId();
    data->processId = GetCurrentProcessId();
    data->fileObject = (ULONG64)*FileHandle;
    data->desiredAccess = (ULONG)DesiredAccess;
    data->fileAttributes = FileAttributes;
    data->shareAccess = ShareAccess;
    data->createDisposition = CreateDisposition;
    data->createOptions = CreateOptions;
    data->status = ret;
    data->fileNameLength = fileName.size();
    wcscpy_s(data->fileName, fileName.data());

    std::string stackTrace = GetConcatStackTraceString();

    // Wrap data and send to pipe server.
    std::vector<std::pair<PVOID, ULONG>> datas = {{data, dataLength}, {(PVOID)stackTrace.data(), stackTrace.size()}};
    SendEvent(datas);
    free(data);

    return ret;
}


NTSTATUS WINAPI DetourNtCreateNamedPipeFile(PHANDLE NamedPipeFileHandle, ULONG DesiredAccess,
                                            POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
                                            ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions,
                                            ULONG NamedPipeType, ULONG ReadMode, ULONG CompletionMode,
                                            ULONG MaximumInstances, ULONG InboundQuota, ULONG OutboundQuota,
                                            PLARGE_INTEGER DefaultTimeout)
{
    NTSTATUS ret = fpNtCreateNamedPipeFile(NamedPipeFileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess,
                                           CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode,
                                           MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);

    // Check if it is a named pipe handle.
    if (GetFileType(*NamedPipeFileHandle) != FILE_TYPE_PIPE)
    {
        return ret;
    }

    // Exclude events made by ourselves.
    std::wstring fileName = GetFileNameFromHandle(*NamedPipeFileHandle);
    if (fileName.find(NAMED_PIPE_MASTER) != std::wstring::npos)
    {
        return ret;
    }

    ULONG dataLength = sizeof(DATA_CREATE_NAMED_PIPE) + 2;
    DATA_CREATE_NAMED_PIPE* data = (DATA_CREATE_NAMED_PIPE*)malloc(dataLength);
    if (!data)
    {
        return ret;
    }
    memset(data, 0, dataLength);

    data->cmdId = CMD_CREATE_NAMED_PIPE;
    data->sourceType = SOURCE_INJECTED_DLL;
    data->threadId = GetCurrentThreadId();
    data->processId = GetCurrentProcessId();
    data->fileObject = (ULONG64)*NamedPipeFileHandle;
    data->desiredAccess = DesiredAccess;
    data->shareAccess = ShareAccess;
    data->createDisposition = CreateDisposition;
    data->createOptions = CreateOptions;
    data->namedPipeType = NamedPipeType;
    data->readMode = ReadMode;
    data->completionMode = CompletionMode;
    data->maximumInstances = MaximumInstances;
    data->inboundQuota = InboundQuota;
    data->outboundQuota = OutboundQuota;
    data->defaultTimeout = DefaultTimeout->QuadPart;
    data->status = ret;
    data->fileNameLength = fileName.size();
    wcscpy_s(data->fileName, fileName.data());

    std::string stackTrace = GetConcatStackTraceString();

    // Wrap data and send to pipe server.
    std::vector<std::pair<PVOID, ULONG>> datas = {{data, dataLength}, {(PVOID)stackTrace.data(), stackTrace.size()}};
    SendEvent(datas);
    free(data);

    return ret;
}


NTSTATUS WINAPI DetourNtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                 PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                 PULONG Key)
{
    NTSTATUS ret = fpNtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

    // Check if it is a named pipe handle.
    if (GetFileType(FileHandle) != FILE_TYPE_PIPE)
    {
        return ret;
    }

    // Exclude events made by ourselves.
    std::wstring fileName = GetFileNameFromHandle(FileHandle);
    if (fileName.find(NAMED_PIPE_MASTER) != std::wstring::npos)
    {
        return ret;
    }

    ULONG copyBufferLength = Length > 1024 ? 1024 : Length;
    ULONG dataLength = sizeof(DATA_READ) + copyBufferLength + 2;
    DATA_READ* data = (DATA_READ*)malloc(dataLength);
    if (!data)
    {
        return ret;
    }
    memset(data, 0, dataLength);

    data->cmdId = CMD_READ;
    data->sourceType = SOURCE_INJECTED_DLL;
    data->threadId = GetCurrentThreadId();
    data->processId = GetCurrentProcessId();
    data->fileObject = (ULONG64)FileHandle;
    data->status = ret;
    data->fileNameLength = fileName.size();
    wcscpy_s(data->fileName, fileName.data());
    if (ImpersonateNamedPipeClient(FileHandle))
    {
        data->canImpersonate = TRUE;
        RevertToSelf();
    }
    else
    {
        data->canImpersonate = FALSE;
    }

    data->readLength = Length;
    memcpy_s(data->readBuffer, copyBufferLength, Buffer, copyBufferLength);

    std::string stackTrace = GetConcatStackTraceString();

    // Wrap data and send to pipe server.
    std::vector<std::pair<PVOID, ULONG>> datas = {{data, dataLength}, {(PVOID)stackTrace.data(), stackTrace.size()}};
    SendEvent(datas);
    free(data);

    return ret;
}


NTSTATUS WINAPI DetourNtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                  PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                  PULONG Key)
{
    NTSTATUS ret = fpNtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

    // Check if it is a named pipe handle.
    if (GetFileType(FileHandle) != FILE_TYPE_PIPE)
    {
        return ret;
    }

    // Exclude events made by ourselves.
    std::wstring fileName = GetFileNameFromHandle(FileHandle);
    if (fileName.find(NAMED_PIPE_MASTER) != std::wstring::npos)
    {
        return ret;
    }

    ULONG copyBufferLength = Length > 1024 ? 1024 : Length;
    ULONG dataLength = sizeof(DATA_WRITE) + copyBufferLength + 2;
    DATA_WRITE* data = (DATA_WRITE*)malloc(dataLength);
    if (!data)
    {
        return ret;
    }
    memset(data, 0, dataLength);

    data->cmdId = CMD_WRITE;
    data->sourceType = SOURCE_INJECTED_DLL;
    data->threadId = GetCurrentThreadId();
    data->processId = GetCurrentProcessId();
    data->fileObject = (ULONG64)FileHandle;
    data->status = ret;
    data->fileNameLength = fileName.size();
    wcscpy_s(data->fileName, fileName.data());
    if (ImpersonateNamedPipeClient(FileHandle))
    {
        data->canImpersonate = TRUE;
        RevertToSelf();
    }
    else
    {
        data->canImpersonate = FALSE;
    }

    data->writeLength = Length;
    memcpy_s(data->writeBuffer, copyBufferLength, Buffer, copyBufferLength);

    std::string stackTrace = GetConcatStackTraceString();

    // Wrap data and send to pipe server.
    std::vector<std::pair<PVOID, ULONG>> datas = {{data, dataLength}, {(PVOID)stackTrace.data(), stackTrace.size()}};
    SendEvent(datas);
    free(data);

    return ret;
}

NTSTATUS NTAPI DetourNtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                     PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer,
                                     ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
    NTSTATUS ret = fpNtFsControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode, InputBuffer,
                                     InputBufferLength, OutputBuffer, OutputBufferLength);

    // Check the caller is from ConnectNamedPipe.
    if (FsControlCode != FSCTL_PIPE_LISTEN)
    {
        return ret;
    }

    // Check if it is a named pipe handle.
    if (GetFileType(FileHandle) != FILE_TYPE_PIPE)
    {
        return ret;
    }

    // Exclude events made by ourselves.
    std::wstring fileName = GetFileNameFromHandle(FileHandle);
    if (fileName.find(NAMED_PIPE_MASTER) != std::wstring::npos)
    {
        return ret;
    }

    ULONG dataLength = sizeof(DATA_CONNECT_NAMED_PIPE) + 2;
    DATA_CONNECT_NAMED_PIPE* data = (DATA_CONNECT_NAMED_PIPE*)malloc(dataLength);
    if (!data)
    {
        return ret;
    }
    memset(data, 0, dataLength);

    data->cmdId = CMD_CONNECT_NAMED_PIPE;
    data->sourceType = SOURCE_INJECTED_DLL;
    data->threadId = GetCurrentThreadId();
    data->processId = GetCurrentProcessId();
    data->fileObject = (ULONG64)FileHandle;
    data->status = ret;
    data->fileNameLength = fileName.size();
    wcscpy_s(data->fileName, fileName.data());
    if (ImpersonateNamedPipeClient(FileHandle))
    {
        data->canImpersonate = TRUE;
        RevertToSelf();
    }
    else
    {
        data->canImpersonate = FALSE;
    }

    std::string stackTrace = GetConcatStackTraceString();

    // Wrap data and send to pipe server.
    std::vector<std::pair<PVOID, ULONG>> datas = {{data, dataLength}, {(PVOID)stackTrace.data(), stackTrace.size()}};
    SendEvent(datas);
    free(data);

    return ret;
}
