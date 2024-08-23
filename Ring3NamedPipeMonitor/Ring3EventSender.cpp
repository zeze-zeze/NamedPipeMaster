#include "Ring3EventSender.h"
#include <Windows.h>
#include <string>
#include <memory>
#include "../NamedPipeMasterBase/defines.h"
#include "../NamedPipeMasterBase/utils.h"

BOOL SendEvent(const std::vector<std::pair<PVOID, ULONG>>& datas)
{
    DWORD dwFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
    std::wstring wsConcat = L"\\\\.\\pipe\\" + std::wstring(NAMED_PIPE_MASTER);
    LPCWSTR pwsPipeName = wsConcat.c_str();
    std::unique_ptr<HANDLE, HandleDeleter> hPipe(
        CreateFile(pwsPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL));
    if (hPipe.get() == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    DWORD bytesWritten;
    for (auto& data : datas)
    {
        if (!WriteFile(hPipe.get(), &data.second, sizeof(data.second), &bytesWritten, NULL))
        {
            return FALSE;
        }

        if (!WriteFile(hPipe.get(), data.first, data.second, &bytesWritten, NULL))
        {
            return FALSE;
        }
    }

    ULONG endLength = 0;
    if (!WriteFile(hPipe.get(), &endLength, sizeof(endLength), &bytesWritten, NULL))
    {
        return FALSE;
    }

    return TRUE;
}
