#include "NamedPipePoker.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <mutex>
#include "../NamedPipeMasterBase/NPMLogger.h"
#include "../NamedPipeMasterBase/utils.h"
#include "../NamedPipeMasterBase/defines.h"

VOID ProxyPokedWorker(const WCHAR* proxyPipeName)
{
    NamedPipePoker namedPipePoker;
    namedPipePoker.proxyPoked(proxyPipeName);
}

VOID NamedPipePoker::PushReadDataQueue(std::vector<CHAR> data)
{
    std::lock_guard<std::mutex> lock(readDataQueueMutex);

    readDataQueue.push(data);
}

std::vector<CHAR> NamedPipePoker::PopReadDataQueue()
{
    std::lock_guard<std::mutex> lock(readDataQueueMutex);

    std::vector<CHAR> data;

    if (readDataQueue.size() == 0)
    {
        return data;
    }

    data = readDataQueue.front();
    readDataQueue.pop();

    return data;
}

ULONG NamedPipePoker::GetReadDataQueueSize()
{
    std::lock_guard<std::mutex> lock(readDataQueueMutex);

    return readDataQueue.size();
}

VOID NamedPipePoker::PrintReadDataQueue()
{
    while (GetReadDataQueueSize() > 0)
    {
        std::vector<CHAR> readData = PopReadDataQueue();
        std::cout << "    Read " << readData.size() << " bytes: \n"
                  << toXxdFormat(reinterpret_cast<const UCHAR*>(readData.data()), readData.size()) << std::endl;
    }
}

BOOL NamedPipePoker::PeekAndRead(const HANDLE& hPipe)
{
    while (TRUE)
    {
        DWORD bytesRead;

        if (PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesRead, NULL))
        {
            // Named pipe still alive, but no data.
            if (bytesRead == 0)
            {
                break;
            }
        }
        else
        {
            // Named pipe has been closed by the other end.
            return FALSE;
        }

        // Read data if any and push to queue.
        std::vector<CHAR> data(bytesRead, 0);
        if (!ReadFile(hPipe, data.data(), data.size(), &bytesRead, NULL))
        {
            return FALSE;
        }

        PushReadDataQueue(data);
    }

    return TRUE;
}

BOOL NamedPipePoker::PokeMenu(const HANDLE& hPipe)
{
    while (1)
    {
        // Peek and read data from named pipe and check if the named pipe is still alive.
        BOOL alive = PeekAndRead(hPipe);

        std::cout << std::endl
                  << "    [a] Write data (alive: " << isTrue(alive) << ")" << std::endl
                  << "    [b] Read data (total " << GetReadDataQueueSize() << " data)" << std::endl
                  << "    [c] Test impersonation" << std::endl
                  << "    [d] exit" << std::endl;
        std::string choice;
        std::cout << "\n    choice: ";
        std::getline(std::cin, choice);
        std::cout << std::endl;

        if (choice == "a")
        {
            std::string writeData;
            std::cout << "    data: ";
            std::getline(std::cin, writeData);

            DWORD bytesWritten;
            if (!WriteFile(hPipe, writeData.data(), writeData.size(), &bytesWritten, NULL))
            {
                // Continue to see if there is still any data to read.
                std::cout << "    Write data error: " << GetLastError() << std::endl;
            }
            else
            {
                std::cout << "    Write data success" << std::endl;
            }
        }
        else if (choice == "b")
        {
            PrintReadDataQueue();
        }
        else if (choice == "c")
        {
            if (ImpersonateNamedPipeClient(hPipe))
            {
                std::cout << "    Impersonate success" << std::endl;
            }
            else
            {
                std::cout << "    Impersonate error: " << GetLastError() << std::endl;
            }

            if (RevertToSelf())
            {
                std::cout << "    RevertToSelf success" << std::endl;
            }
            else
            {
                std::cout << "    RevertToSelf error: " << GetLastError() << std::endl;
            }
        }
        else if (choice == "d")
        {
            break;
        }
        else if (choice.empty())
        {
            continue;
        }
        else
        {
            std::cout << "    invalid choice" << std::endl;
        }
    }

    std::cout << "    named pipe communication over" << std::endl;
    return TRUE;
}

BOOL NamedPipePoker::Poke(const std::wstring& pipeName)
{
    DWORD dwFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
    std::unique_ptr<HANDLE, HandleDeleter> hPipe(
        CreateFile(pipeName.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL));
    if (hPipe.get() == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateFile error: " << GetLastError() << std::endl;
        return FALSE;
    }

    PokeMenu(hPipe.get());

    return TRUE;
}

BOOL NamedPipePoker::Poked(const std::wstring& pipeName)
{
    DWORD dwFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
    std::unique_ptr<HANDLE, HandleDeleter> hPipe(CreateNamedPipe(pipeName.data(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                                                 PIPE_TYPE_MESSAGE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES,
                                                                 2048, 2048, 0, NULL));
    if (hPipe.get() == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateNamedPipe error: " << GetLastError() << std::endl;
        return FALSE;
    }

    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!overlapped.hEvent)
    {
        std::cout << "CreateEvent error: " << GetLastError() << std::endl;
        return FALSE;
    }

    BOOL isConnected = ConnectNamedPipe(hPipe.get(), &overlapped);
    if (!isConnected && GetLastError() != ERROR_PIPE_LISTENING)
    {
        std::cout << "ConnectNamedPipe error: " << GetLastError() << std::endl;
        return FALSE;
    }

    PokeMenu(hPipe.get());

    return TRUE;
}

BOOL NamedPipePoker::proxyPoke(const std::wstring& proxyPipeName, const DWORD& pid, const std::wstring& targetPipeName)
{
    // Inject into the proxy process and let it open a named pipe server.
    if (DllInject(toUTF8(GetCurrentDirPath() + INJECTED_DLL_NAME + L".dll"), pid))
    {
        std::cout << "Inject pid " << pid << " successfully." << std::endl;
    }
    else
    {
        std::cout << "Inject pid " << pid << " error." << std::endl;
        return FALSE;
    }

    // Wait for the proxy process to setup named pipe.
    Sleep(1000);

    // Send PokeCreateInfo to the proxy process.
    DWORD dwFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
    std::unique_ptr<HANDLE, HandleDeleter> hPipe(
        CreateFile(proxyPipeName.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL));
    if (hPipe.get() == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateFile error: " << GetLastError();
        return FALSE;
    }

    PokeCreateInfo pokeCreateInfo = {0};
    pokeCreateInfo.targetPipeNameLength = targetPipeName.size();
    wcsncpy_s(pokeCreateInfo.targetPipeName, targetPipeName.data(), targetPipeName.size());
    DWORD bytesWritten;
    if (!WriteFile(hPipe.get(), &pokeCreateInfo, sizeof(pokeCreateInfo), &bytesWritten, NULL))
    {
        std::cout << "WriteFile error: " << GetLastError();
        return FALSE;
    }

    // Communicate with the target process through proxy target process.
    PokeMenu(hPipe.get());

    return TRUE;
}

BOOL NamedPipePoker::RelayPipeData(const HANDLE& fromPipe, const HANDLE& toPipe)
{
    DWORD bytesRead;

    if (PeekNamedPipe(fromPipe, NULL, 0, NULL, &bytesRead, NULL))
    {
        // Named pipe still alive, but no data.
        if (bytesRead == 0)
        {
            return TRUE;
        }
    }
    else
    {
        // Named pipe has been closed by the other end.
        return FALSE;
    }

    // Read data from fromPipe.
    std::vector<CHAR> data(bytesRead, 0);
    if (!ReadFile(fromPipe, data.data(), data.size(), &bytesRead, NULL))
    {
        return FALSE;
    }

    // Write data to toPipe.
    DWORD bytesWritten;
    if (!WriteFile(toPipe, data.data(), data.size(), &bytesWritten, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL NamedPipePoker::proxyPoked(const std::wstring& proxyPipeName)
{
    while (TRUE)
    {
        // Communicate with NamedPipeConsumer.
        std::unique_ptr<HANDLE, HandleDeleter> hProxyPipe(CreateNamedPipe((L"\\\\.\\pipe\\" + proxyPipeName).data(),
                                                                          PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE,
                                                                          PIPE_UNLIMITED_INSTANCES, 2048, 2048, 0, NULL));
        if (hProxyPipe.get() == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        if (!ConnectNamedPipe(hProxyPipe.get(), NULL))
        {
            continue;
        }

        DWORD bytesRead;
        std::vector<CHAR> data(sizeof(PokeCreateInfo), 0);
        if (!ReadFile(hProxyPipe.get(), data.data(), data.size(), &bytesRead, NULL))
        {
            continue;
        }
        PokeCreateInfo* pokeCreateInfo = (PokeCreateInfo*)data.data();

        // Communicate with target pipe server.
        std::wstring targetPipeName(pokeCreateInfo->targetPipeName, pokeCreateInfo->targetPipeNameLength);
        DWORD dwFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
        std::unique_ptr<HANDLE, HandleDeleter> hTargetPipe(
            CreateFile(targetPipeName.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL));
        if (hTargetPipe.get() == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        // Relay data between pipes.
        BOOL status = TRUE;
        while (status)
        {
            status &= RelayPipeData(hTargetPipe.get(), hProxyPipe.get());
            status &= RelayPipeData(hProxyPipe.get(), hTargetPipe.get());
            Sleep(1000);
        }
    }

    return TRUE;
}
