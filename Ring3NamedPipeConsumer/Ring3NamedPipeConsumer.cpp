#include <Windows.h>
#include <string>
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <mutex>
#include "../NamedPipeMasterBase/utils.h"
#include "EventReceiver.h"
#include "../NamedPipeMasterBase/NamedPipeEventDatabase.h"
#include "../NamedPipeMasterBase/NPMLogger.h"
#include "../NamedPipeMasterBase/defines.h"
#include "../NamedPipeMasterBase/NamedPipePoker.h"

VOID Menu();
int SetOptions(int argc, char** argv);
BOOL CleanUp();
BOOL CtrlHandler(DWORD fdwCtrlType);
DWORD namedPipeThreadNum = 4;
std::mutex exitMutex;

VOID Menu()
{
    auto startTime = std::chrono::steady_clock::now();
    bool detail_help = false;

    while (TRUE)
    {
        if (!detail_help)
        {
            std::cout << "[1] dump database" << std::endl
                      << "[2] start monitor mode" << std::endl
                      << "[3] clear database" << std::endl
                      << "[4] get database info" << std::endl
                      << "[5] filter" << std::endl
                      << "[6] inject dll" << std::endl
                      << "[7] NamedPipePoker" << std::endl
                      << "[8] NamedPipeProxyPoker" << std::endl
                      << "[9] NamedPipePoked" << std::endl
                      << "[10] help" << std::endl
                      << "[11] exit and clean up" << std::endl
                      << std::endl;
        }
        else
        {
            detail_help = false;
        }

        std::string line = "";
        while (line.empty())
        {
            std::cout << "NPM-CLI> ";
            std::getline(std::cin, line);
            if (std::cin.fail() || std::cin.eof())
            {
                return;
            }
        }
        DWORD choice = atoi(line.data());

        if (choice == 1)
        {
            for (auto e : NamedPipeEventDatabase::Instance().SelectDatabase())
            {
                NamedPipeEventDatabase::Instance().PrintEvent(e, TRUE);
            }
        }
        else if (choice == 2)
        {
            NamedPipeEventDatabase::Instance().SetMonitorMode(TRUE);
            std::cout << "Press enter to stop monitoring ...\n";
            getchar();
            NamedPipeEventDatabase::Instance().SetMonitorMode(FALSE);
        }
        else if (choice == 3)
        {
            NamedPipeEventDatabase::Instance().ClearDatabase();
        }
        else if (choice == 4)
        {
            // Get running time from start in seconds.
            auto endTime = std::chrono::steady_clock::now();
            DWORD duration = (DWORD)std::chrono::duration<double>(endTime - startTime).count();
            NPMLogger::LoggerInstance()->info("time monitoring: {} secocnds", duration);

            // Get the size of database and capacity of database.
            NPMLogger::LoggerInstance()->info("size of database: {}", NamedPipeEventDatabase::Instance().GetDatabaseSize());
            NPMLogger::LoggerInstance()->info("capacity of database: {}", NamedPipeEventDatabase::Instance().GetMaxDbSize());

            // Get unique pairs of image path and file name.
            nlohmann::json uniqueImagePathAndFileNamePairs(
                NamedPipeEventDatabase::Instance().GetUniqueImagePathAndFileNamePairs());
            NPMLogger::LoggerInstance()->info("unique pairs of [image path, file name]: {}",
                                              uniqueImagePathAndFileNamePairs.dump(4));
        }
        else if (choice == 5)
        {
            NAMED_PIPE_EVENT_FILTER filter = NamedPipeEventDatabase::Instance().GetFilter();

            std::cout << "    [a] Filter cmdId: " << (filter.cmdId == 0 ? "all" : std::to_string(filter.cmdId))
                      << " (0=all, 1=CMD_CREATE_NAMED_PIPE, 2=CMD_CREATE, 3=CMD_READ, 4=CMD_WRITE, 5=CMD_CONNECT_NAMED_PIPE)"
                      << std::endl
                      << "    [b] Filter sourceType: "
                      << (filter.sourceType == 0 ? "all" : std::to_string(filter.sourceType))
                      << " (0=all, 1=SOURCE_MINIFILTER, 2=SOURCE_INJECTED_DLL)" << std::endl
                      << "    [c] Filter processId: " << (filter.processId == 0 ? "all" : std::to_string(filter.processId))
                      << " (0=all)" << std::endl
                      << "    [d] Filter imagePath: %" << filter.imagePath << "%" << std::endl
                      << "    [e] Filter fileName: %" << filter.fileName << "%" << std::endl
                      << "    [f] Filter canImpersonate: " << (filter.canImpersonate ? "yes" : "all") << " (0=all, 1=yes)"
                      << std::endl
                      << "    [g] Reset filter" << std::endl
                      << std::endl;

            std::string choice;
            std::cout << "    choice: ";
            std::getline(std::cin, choice);

            if (choice == "a")
            {
                std::string cmdId;
                std::cout << "    cmdId: ";
                std::getline(std::cin, cmdId);
                filter.cmdId = atoi(cmdId.data());
            }
            else if (choice == "b")
            {
                std::string sourceType;
                std::cout << "    sourceType: ";
                std::getline(std::cin, sourceType);
                filter.sourceType = atoi(sourceType.data());
            }
            else if (choice == "c")
            {
                std::string processId;
                std::cout << "    processId: ";
                std::getline(std::cin, processId);
                filter.processId = atoi(processId.data());
            }
            else if (choice == "d")
            {
                std::string imagePath;
                std::cout << "    imagePath: ";
                std::getline(std::cin, imagePath);
                filter.imagePath = imagePath;
            }
            else if (choice == "e")
            {
                std::string fileName;
                std::cout << "    fileName: ";
                std::getline(std::cin, fileName);
                filter.fileName = fileName;
            }
            else if (choice == "f")
            {
                std::string canImpersonate;
                std::cout << "    canImpersonate: ";
                std::getline(std::cin, canImpersonate);
                filter.canImpersonate = atoi(canImpersonate.data()) == 0 ? FALSE : TRUE;
            }
            else if (choice == "g")
            {
                NamedPipeEventDatabase::Instance().ResetFilter();
                filter = NamedPipeEventDatabase::Instance().GetFilter();
            }
            else
            {
                std::cout << "    invalid filter" << std::endl;
            }

            NamedPipeEventDatabase::Instance().SetFilter(filter);
        }
        else if (choice == 6)
        {
            std::string pid;
            std::cout << "target pid: ";
            std::getline(std::cin, pid);
            if (DllInject(toUTF8(GetCurrentDirPath() + INJECTED_DLL_NAME + L".dll"), atoi(pid.data())))
            {
                std::cout << "Inject pid " << pid << " successfully." << std::endl;
            }
            else
            {
                std::cout << "Inject pid " << pid << " error." << std::endl;
            }
        }
        else if (choice == 7)
        {
            std::string pipeName;
            std::cout << "pipe name: ";
            std::getline(std::cin, pipeName);

            NamedPipePoker namedPipePoker;
            namedPipePoker.Poke(toUTF16(pipeName));
        }
        else if (choice == 8)
        {
            std::string pid;
            std::cout << "target pid: ";
            std::getline(std::cin, pid);
            if (atoi(pid.data()) == GetCurrentProcessId())
            {
                std::cout << "Don't treat self as a proxy." << std::endl;
                continue;
            }

            std::string targetPipeName;
            std::cout << "target pipe name: ";
            std::getline(std::cin, targetPipeName);

            NamedPipePoker namedPipePoker;
            namedPipePoker.proxyPoke(std::wstring(L"\\\\.\\pipe\\") + NAMED_PIPE_MONITOR + toUTF16(pid), atoi(pid.data()),
                                     toUTF16(targetPipeName));
        }
        else if (choice == 9)
        {
            std::string pipeName;
            std::cout << "pipe name: ";
            std::getline(std::cin, pipeName);

            NamedPipePoker namedPipePoker;
            namedPipePoker.Poked(toUTF16(pipeName));
        }
        else if (choice == 10)
        {
            detail_help = true;
            std::cout << "[1] dump database: print all monitored events in the log" << std::endl
                      << "[2] start monitor mode: keep monitoring named pipe activities until enter is pressed" << std::endl
                      << "[3] clear database: clear the log" << std::endl
                      << "[4] get database info: get some statistics" << std::endl
                      << "[5] filter: get the specified named pipe events" << std::endl
                      << "[6] inject dll: inject Ring3NamedPipeMonitor.dll into a process" << std::endl
                      << "[7] NamedPipePoker: directly interact with a named pipe server" << std::endl
                      << "[8] NamedPipeProxyPoker: inject Ring3NamedPipeMonitor.dll into a process as a proxy to interact "
                         "with the target named pipe server"
                      << std::endl
                      << "[9] NamedPipePoked: act as a named pipe server to be connected by other clients" << std::endl
                      << "[10] help: print this detail usage" << std::endl
                      << "[11] exit and clean up: terminate this process and unload the driver" << std::endl
                      << std::endl;
            continue;
        }
        else if (choice == 11)
        {
            return;
        }
        else
        {
            std::cout << "invalid command" << std::endl;
        }

        std::cout << std::endl;
    }
}

int SetOptions(int argc, char** argv)
{
    CLI::App app {"Named Pipe Master"};
    DWORD logLevel = 2;
    app.add_option("-l,--log_level", logLevel, "0=trace, 1=debug, 2=info, 3=warn, 4=error, 5=critical (default 2)");
    DWORD maxDbSize = 65535;
    app.add_option("-m,--max_db_size", maxDbSize, "the storage for database to store events (default 65535)");
    app.add_option("-t,--thread", namedPipeThreadNum, "the number of named pipe thread (default 4)");
    CLI11_PARSE(app, argc, argv);

    NPMLogger::Instance().SetLogLevel((spdlog::level::level_enum)logLevel);
    NamedPipeEventDatabase::Instance().SetMaxDbSize(maxDbSize);

    return 0;
}

BOOL CleanUp()
{
    if (!UnloadDriver(FILTER_DRIVER_NAME))
    {
        NPMLogger::LoggerInstance()->warn("UnloadDriver error");
        return FALSE;
    }
    return TRUE;
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        case CTRL_C_EVENT:
        {
            std::lock_guard<std::mutex> lock(exitMutex);
            CleanUp();
            ExitProcess(0);
            break;
        }
        default:
            return FALSE;
    }

    return TRUE;
}

int main(int argc, char** argv)
{
    SetOptions(argc, argv);

    // Setup UEF (Unhandled Exception Filter).
    LPTOP_LEVEL_EXCEPTION_FILTER filter = SetUnhandledExceptionFilter(UefExceptionFilter);
    if (!filter)
    {
        NPMLogger::LoggerInstance()->error("[{}] SetUnhandledExceptionFilter error", __FUNCTION__);
        return 1;
    }

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
    {
        NPMLogger::LoggerInstance()->error("[{}] SetConsoleCtrlHandler error", __FUNCTION__);
        return 1;
    }

    // Initialize sqlite_orm to store named pipe events.
    if (!NamedPipeEventDatabase::Instance().InitializeDatabase())
    {
        NPMLogger::LoggerInstance()->error("[{}] InitializeDatabase error", __FUNCTION__);
        return 1;
    }

    // Load minifilter driver to monitor named pipe events.
    EnableDebugPrivilege();
    if (IsProcessElevated() &&
        LoadDriver(FILTER_DRIVER_NAME, (GetCurrentDirPath() + FILTER_DRIVER_NAME + L".sys").data(), FILTER_DRIVER_ALTITUDE))
    {
        // Create a thread for filter port.
        std::unique_ptr<HANDLE, HandleDeleter> hThread(
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateMinifilterCommunication, NULL, 0, NULL));
        if (hThread.get() == NULL)
        {
            NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
            return 1;
        }
    }
    else
    {
        NPMLogger::LoggerInstance()->warn("LoadDriver error. Start without {}", toUTF8(FILTER_DRIVER_NAME));
    }

    // Create 4 threads to handle named pipe connections.
    DWORD namedPipeThreadCount = 0;
    while (namedPipeThreadCount < namedPipeThreadNum)
    {
        namedPipeThreadCount++;
        std::unique_ptr<HANDLE, HandleDeleter> hThread(
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateNamedPipeCommunication, NULL, 0, NULL));
        if (hThread.get() == NULL)
        {
            namedPipeThreadCount--;
            NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
            continue;
        }
    }

    // Command line user interface.
    Menu();

    // Unload minifilter driver when exit.
    std::lock_guard<std::mutex> lock(exitMutex);
    CleanUp();
    ExitProcess(0);

    return 0;
}
