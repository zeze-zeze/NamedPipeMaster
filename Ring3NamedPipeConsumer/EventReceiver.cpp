#include "EventReceiver.h"
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <fltUser.h>
#include "../NamedPipeMasterBase/utils.h"
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"
#include "../NamedPipeMasterBase/NPMLogger.h"
#include "../NamedPipeMasterBase/NamedPipeEventDatabase.h"

BOOL ParseDataCreateNamedPipe(DATA_CREATE_NAMED_PIPE* data, std::string stackTrace)
{
    NAMED_PIPE_EVENT namedPipeEvent = {0};
    namedPipeEvent.currentTime = GetCurrentTimeStr();
    namedPipeEvent.cmdId = data->cmdId;
    namedPipeEvent.sourceType = data->sourceType;
    namedPipeEvent.threadId = data->threadId;
    namedPipeEvent.processId = data->processId;
    namedPipeEvent.imagePath = GetProcessPathByPid(data->processId);
    namedPipeEvent.fileObject = data->fileObject;
    namedPipeEvent.desiredAccess = data->desiredAccess;
    namedPipeEvent.shareAccess = data->shareAccess;
    namedPipeEvent.createDisposition = data->createDisposition;
    namedPipeEvent.createOptions = data->createOptions;
    namedPipeEvent.namedPipeType = data->namedPipeType;
    namedPipeEvent.readMode = data->readMode;
    namedPipeEvent.completionMode = data->completionMode;
    namedPipeEvent.maximumInstances = data->maximumInstances;
    namedPipeEvent.inboundQuota = data->inboundQuota;
    namedPipeEvent.outboundQuota = data->outboundQuota;
    namedPipeEvent.defaultTimeout = data->defaultTimeout;
    namedPipeEvent.status = data->status;
    namedPipeEvent.fileNameLength = data->fileNameLength;
    namedPipeEvent.fileName = toUTF8(std::wstring(data->fileName));
    namedPipeEvent.stackTrace = stackTrace;

    NamedPipeEventDatabase::Instance().StoreEvent(namedPipeEvent);

    return TRUE;
}

BOOL ParseDataCreate(DATA_CREATE* data, std::string stackTrace)
{
    NAMED_PIPE_EVENT namedPipeEvent = {0};
    namedPipeEvent.currentTime = GetCurrentTimeStr();
    namedPipeEvent.cmdId = data->cmdId;
    namedPipeEvent.sourceType = data->sourceType;
    namedPipeEvent.threadId = data->threadId;
    namedPipeEvent.processId = data->processId;
    namedPipeEvent.imagePath = GetProcessPathByPid(data->processId);
    namedPipeEvent.fileObject = data->fileObject;
    namedPipeEvent.desiredAccess = data->desiredAccess;
    namedPipeEvent.fileAttributes = data->fileAttributes;
    namedPipeEvent.shareAccess = data->shareAccess;
    namedPipeEvent.createDisposition = data->createDisposition;
    namedPipeEvent.createOptions = data->createOptions;
    namedPipeEvent.status = data->status;
    namedPipeEvent.fileNameLength = data->fileNameLength;
    namedPipeEvent.fileName = toUTF8(std::wstring(data->fileName));
    namedPipeEvent.stackTrace = stackTrace;

    NamedPipeEventDatabase::Instance().StoreEvent(namedPipeEvent);

    return TRUE;
}

BOOL ParseDataRead(DATA_READ* data, std::string stackTrace)
{
    NAMED_PIPE_EVENT namedPipeEvent = {0};
    namedPipeEvent.currentTime = GetCurrentTimeStr();
    namedPipeEvent.cmdId = data->cmdId;
    namedPipeEvent.sourceType = data->sourceType;
    namedPipeEvent.threadId = data->threadId;
    namedPipeEvent.processId = data->processId;
    namedPipeEvent.imagePath = GetProcessPathByPid(data->processId);
    namedPipeEvent.fileObject = data->fileObject;
    namedPipeEvent.status = data->status;
    namedPipeEvent.fileNameLength = data->fileNameLength;
    namedPipeEvent.fileName = toUTF8(std::wstring(data->fileName));
    namedPipeEvent.canImpersonate = data->canImpersonate;
    namedPipeEvent.bufferLength = data->readLength;
    ULONG copyReadLength =
        data->readLength > COPY_READ_WRITE_BUFFER_LENGTH ? COPY_READ_WRITE_BUFFER_LENGTH : data->readLength;
    std::vector<CHAR> buffer(data->readBuffer, data->readBuffer + copyReadLength);
    namedPipeEvent.buffer = buffer;
    namedPipeEvent.stackTrace = stackTrace;

    NamedPipeEventDatabase::Instance().StoreEvent(namedPipeEvent);

    return TRUE;
}

BOOL ParseDataWrite(DATA_WRITE* data, std::string stackTrace)
{
    NAMED_PIPE_EVENT namedPipeEvent = {0};
    namedPipeEvent.currentTime = GetCurrentTimeStr();
    namedPipeEvent.cmdId = data->cmdId;
    namedPipeEvent.sourceType = data->sourceType;
    namedPipeEvent.threadId = data->threadId;
    namedPipeEvent.processId = data->processId;
    namedPipeEvent.imagePath = GetProcessPathByPid(data->processId);
    namedPipeEvent.fileObject = data->fileObject;
    namedPipeEvent.status = data->status;
    namedPipeEvent.fileNameLength = data->fileNameLength;
    namedPipeEvent.fileName = toUTF8(std::wstring(data->fileName));
    namedPipeEvent.canImpersonate = data->canImpersonate;
    namedPipeEvent.bufferLength = data->writeLength;
    ULONG copyWriteLength =
        data->writeLength > COPY_READ_WRITE_BUFFER_LENGTH ? COPY_READ_WRITE_BUFFER_LENGTH : data->writeLength;
    std::vector<CHAR> buffer(data->writeBuffer, data->writeBuffer + copyWriteLength);
    namedPipeEvent.buffer = buffer;
    namedPipeEvent.stackTrace = stackTrace;

    NamedPipeEventDatabase::Instance().StoreEvent(namedPipeEvent);

    return TRUE;
}

BOOL ParseDataConnectNamedPipe(DATA_CONNECT_NAMED_PIPE* data, std::string stackTrace)
{
    NAMED_PIPE_EVENT namedPipeEvent = {0};
    namedPipeEvent.currentTime = GetCurrentTimeStr();
    namedPipeEvent.cmdId = data->cmdId;
    namedPipeEvent.sourceType = data->sourceType;
    namedPipeEvent.threadId = data->threadId;
    namedPipeEvent.processId = data->processId;
    namedPipeEvent.imagePath = GetProcessPathByPid(data->processId);
    namedPipeEvent.fileObject = data->fileObject;
    namedPipeEvent.status = data->status;
    namedPipeEvent.fileNameLength = data->fileNameLength;
    namedPipeEvent.fileName = toUTF8(std::wstring(data->fileName));
    namedPipeEvent.canImpersonate = data->canImpersonate;
    namedPipeEvent.stackTrace = stackTrace;

    NamedPipeEventDatabase::Instance().StoreEvent(namedPipeEvent);

    return TRUE;
}

BOOL HandleEvent(std::vector<std::pair<PVOID, ULONG>> datas)
{
    if (datas.empty())
    {
        return FALSE;
    }

    BOOL ret = TRUE;

    // Get stack trace in the second index of datas.
    std::string stackTrace = "";
    if (datas.size() > 1)
    {
        stackTrace = std::string((CHAR*)datas[1].first, datas[1].second);
    }

    // Get cmdId in the first 4 bytes of the first index of datas.
    ULONG cmdId = *(ULONG*)datas[0].first;
    switch (cmdId)
    {
        case CMD_CREATE_NAMED_PIPE:
        {
            ParseDataCreateNamedPipe((DATA_CREATE_NAMED_PIPE*)datas[0].first, stackTrace);
            break;
        }
        case CMD_CREATE:
        {
            ParseDataCreate((DATA_CREATE*)datas[0].first, stackTrace);
            break;
        }
        case CMD_READ:
        {
            ParseDataRead((DATA_READ*)datas[0].first, stackTrace);
            break;
        }
        case CMD_WRITE:
        {
            ParseDataWrite((DATA_WRITE*)datas[0].first, stackTrace);
            break;
        }
        case CMD_CONNECT_NAMED_PIPE:
        {
            ParseDataConnectNamedPipe((DATA_CONNECT_NAMED_PIPE*)datas[0].first, stackTrace);
            break;
        }
        default:
        {
            NPMLogger::LoggerInstance()->warn("Unknown command: {}", cmdId);
            ret = FALSE;
        }
    }

    return ret;
}

VOID HandleNamedPipeClientConnection(HANDLE hServerPipe)
{
    std::vector<std::pair<PVOID, ULONG>> datas;

    while (1)
    {
        ULONG dataLength;
        DWORD dwBytesRead;

        // Get length of the data from pipe client.
        if (!ReadFile(hServerPipe, &dataLength, sizeof(dataLength), &dwBytesRead, NULL))
        {
            NPMLogger::LoggerInstance()->error("[{}] ReadFile error: {}", __FUNCTION__, GetLastError());
            goto _EXIT;
        }

        // Break the loop when receiving 0.
        if (dataLength == 0)
        {
            break;
        }

        // Allocate memory with the length from pipe client.
        PVOID data = malloc(dataLength);
        if (!data)
        {
            NPMLogger::LoggerInstance()->error("[{}] malloc error", __FUNCTION__);
            goto _EXIT;
        }
        memset(data, 0, dataLength);
        datas.push_back({data, dataLength});

        // Get the data from pipe client.
        if (!ReadFile(hServerPipe, data, dataLength, &dwBytesRead, NULL))
        {
            NPMLogger::LoggerInstance()->error("[{}] ReadFile error: {}", __FUNCTION__, GetLastError());
            goto _EXIT;
        }
    }

    if (!HandleEvent(datas))
    {
        NPMLogger::LoggerInstance()->warn("HandleEvent error");
    }

_EXIT:
    if (hServerPipe)
    {
        DisconnectNamedPipe(hServerPipe);
        CloseHandle(hServerPipe);
    }
    for (auto& data : datas)
    {
        free(data.first);
    }
}

BOOL CreateNamedPipeCommunication()
{
    // Set security descriptor to let everyone access the named pipe.
    SECURITY_DESCRIPTOR sd = {0};
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
        NPMLogger::LoggerInstance()->error("[{}] InitializeSecurityDescriptor error: {}", __FUNCTION__, GetLastError());
        return FALSE;
    }

    if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
    {
        NPMLogger::LoggerInstance()->error("[{}] SetSecurityDescriptorDacl error: {}", __FUNCTION__, GetLastError());
        return FALSE;
    }

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    std::wstring wsConcat = L"\\\\.\\pipe\\" + std::wstring(NAMED_PIPE_MASTER);
    LPCWSTR pwsPipeName = wsConcat.c_str();

    while (TRUE)
    {
        HANDLE hServerPipe = CreateNamedPipe(pwsPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
                                             2048, 2048, 200, &sa);
        if (hServerPipe == INVALID_HANDLE_VALUE)
        {
            NPMLogger::LoggerInstance()->error("[{}] CreateNamedPipe error: {}", __FUNCTION__, GetLastError());
            continue;
        }

        if (!ConnectNamedPipe(hServerPipe, NULL))
        {
            DWORD error = GetLastError();

            // Sometimes there are many client connections that may cause ERROR_PIPE_CONNECTED due to race condition.
            // This can happen if a client connects in the interval between the call to CreateNamedPipe and the call to
            // ConnectNamedPipe. In this situation, there is a good connection between client and server.
            if (error == ERROR_PIPE_CONNECTED) {}
            else
            {
                // ERROR_NO_DATA happens frequently if the previous client has closed its handle.
                if (error == ERROR_NO_DATA)
                {
                    // NPMLogger::LoggerInstance()->warn("ConnectNamedPipe error: {}", __FUNCTION__, GetLastError());
                }
                else
                {
                    NPMLogger::LoggerInstance()->error("[{}] ConnectNamedPipe error: {}", __FUNCTION__, GetLastError());
                }

                DisconnectNamedPipe(hServerPipe);
                CloseHandle(hServerPipe);
                continue;
            }
        }

        // Create a thread to handle the client connection.
        HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HandleNamedPipeClientConnection, hServerPipe, 0, NULL);
        if (thread == NULL)
        {
            NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
            DisconnectNamedPipe(hServerPipe);
            CloseHandle(hServerPipe);
            continue;
        }

        CloseHandle(thread);
    }
}

BOOL CreateMinifilterCommunication()
{
    HRESULT hResult;
    try
    {
        HANDLE hPort;
        hResult = FilterConnectCommunicationPort(NAMED_PIPE_MASTER_PORT_NAME, 0, nullptr, 0, nullptr, &hPort);
        if (IS_ERROR(hResult))
        {
            NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
            return FALSE;
        }
        std::unique_ptr<HANDLE, HandleDeleter> upPort(hPort);

        while (TRUE)
        {
            NAMED_PIPE_MASTER_FLT_WRAPPER namedPipeMasterFltWrapper = {0};

            hResult = FilterGetMessage(upPort.get(), &namedPipeMasterFltWrapper.hdr, MAX_BUFFER_SIZE, nullptr);
            if (IS_ERROR(hResult))
            {
                NPMLogger::LoggerInstance()->error("[{}] FilterGetMessage error: {}", __FUNCTION__, hResult);
                continue;
            }

            std::vector<std::pair<PVOID, ULONG>> datas = {{namedPipeMasterFltWrapper.data, MAX_BUFFER_SIZE}};

            if (!HandleEvent(datas))
            {
                NPMLogger::LoggerInstance()->warn("HandleEvent error");
                continue;
            }
        }
    }
    catch (const std::exception& e)
    {
        NPMLogger::LoggerInstance()->error("[{}] CreateMinifilterCommunication error: {}", __FUNCTION__, e.what());
        return FALSE;
    }

    return TRUE;
}
