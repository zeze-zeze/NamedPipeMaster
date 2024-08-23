#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <fltUser.h>
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"

BOOL ParseDataCreateNamedPipe(DATA_CREATE_NAMED_PIPE* data, std::string stackTrace);
BOOL ParseDataCreate(DATA_CREATE* data, std::string stackTrace);
BOOL ParseDataRead(DATA_READ* data, std::string stackTrace);
BOOL ParseDataWrite(DATA_WRITE* data, std::string stackTrace);
BOOL ParseDataConnectNamedPipe(DATA_CONNECT_NAMED_PIPE* data, std::string stackTrace);
BOOL HandleEvent(std::vector<std::pair<PVOID, ULONG>> datas);
VOID HandleNamedPipeClientConnection(HANDLE hServerPipe);
BOOL CreateNamedPipeCommunication();
BOOL CreateMinifilterCommunication();

// The data from kernel must smaller than 0x1000.
struct NAMED_PIPE_MASTER_FLT_WRAPPER
{
    FILTER_MESSAGE_HEADER hdr;
    CHAR data[MAX_BUFFER_SIZE];
};
