#pragma once
#if defined(Ring0NamedPipeFilter)
#include <ntddk.h>
#elif defined(Ring3NamedPipeConsumer)
#include <Windows.h>
#else
#include <Windows.h>
#endif

#include "defines.h"

enum CMD_ID
{
    CMD_UNKNOWN = -1,
    CMD_CREATE_NAMED_PIPE = 1,
    CMD_CREATE = 2,
    CMD_READ,
    CMD_WRITE,
    CMD_CONNECT_NAMED_PIPE
};

enum SOURCE_TYPE
{
    SOURCE_UNKNOWN = -1,
    SOURCE_MINIFILTER = 1,
    SOURCE_INJECTED_DLL = 2
};

struct DATA_CREATE_NAMED_PIPE
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG threadId;
    ULONG processId;
    ULONG64 fileObject;
    ULONG desiredAccess;
    ULONG shareAccess;
    ULONG createDisposition;
    ULONG createOptions;
    ULONG namedPipeType;
    ULONG readMode;
    ULONG completionMode;
    ULONG maximumInstances;
    ULONG inboundQuota;
    ULONG outboundQuota;
    ULONG64 defaultTimeout;
    ULONG status;
    ULONG fileNameLength;
    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
};

struct DATA_CREATE
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG threadId;
    ULONG processId;
    ULONG64 fileObject;
    ULONG desiredAccess;
    ULONG fileAttributes;
    ULONG shareAccess;
    ULONG createDisposition;
    ULONG createOptions;
    ULONG status;
    ULONG fileNameLength;
    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
};

struct DATA_READ
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG threadId;
    ULONG processId;
    ULONG64 fileObject;
    ULONG status;
    ULONG fileNameLength;
    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
    bool canImpersonate;
    ULONG readLength;
    UCHAR readBuffer[1];
};

struct DATA_WRITE
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG threadId;
    ULONG processId;
    ULONG64 fileObject;
    ULONG status;
    ULONG fileNameLength;
    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
    bool canImpersonate;
    ULONG writeLength;
    UCHAR writeBuffer[1];
};

struct DATA_CONNECT_NAMED_PIPE
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG threadId;
    ULONG processId;
    ULONG64 fileObject;
    ULONG status;
    ULONG fileNameLength;
    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1];
    bool canImpersonate;
};
