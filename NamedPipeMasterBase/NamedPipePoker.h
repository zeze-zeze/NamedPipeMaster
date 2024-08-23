#pragma once
#include <Windows.h>
#include <queue>
#include <vector>
#include <mutex>
#include "../NamedPipeMasterBase/defines.h"

enum PokeCmdId
{
    POKE_CMD_UNKNOWN = -1,
    POKE_CMD_CREATE = 1,
    POKE_CMD_READ = 2,
    POKE_CMD_WRITE
};

struct PokeCreateInfo
{
    ULONG pokeCmdId;
    ULONG targetPipeNameLength;
    WCHAR targetPipeName[MAX_PIPE_NAME_LENGTH + 1];
};

struct PokeReadWriteInfo
{
    ULONG pokeCmdId;
    ULONG bufferLength;
    WCHAR buffer[1];
};

VOID ProxyPokedWorker(const WCHAR* proxyPipeName);

class NamedPipePoker
{
  private:
    std::mutex readDataQueueMutex;
    std::queue<std::vector<CHAR>> readDataQueue;

  private:
    VOID PrintReadDataQueue();
    BOOL PokeMenu(const HANDLE& hPipe);
    VOID PushReadDataQueue(std::vector<CHAR> data);
    std::vector<CHAR> PopReadDataQueue();
    ULONG GetReadDataQueueSize();
    BOOL PeekAndRead(const HANDLE& hPipe);
    BOOL RelayPipeData(const HANDLE& fromPipe, const HANDLE& toPipe);

  public:
    BOOL Poke(const std::wstring& pipeName);
    BOOL Poked(const std::wstring& pipeName);
    BOOL proxyPoke(const std::wstring& proxyPipeName, const DWORD& pid, const std::wstring& targetPipeName);
    BOOL proxyPoked(const std::wstring& proxyPipeName);
};
