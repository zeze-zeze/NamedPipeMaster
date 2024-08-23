#include <MinHook.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#include "../NamedPipeMasterBase/utils.h"
#include "../NamedPipeMasterBase/NamedPipePoker.h"
#include "DetourFunction.h"


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (MH_Initialize() != MH_OK)
            {
                return FALSE;
            }

            // NtCreateFile
            tNtCreateFile ntdllNtCreateFile = (tNtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll"), "NtCreateFile");
            if (!ntdllNtCreateFile)
            {
                return FALSE;
            }

            if (MH_CreateHook((LPVOID)ntdllNtCreateFile, &DetourNtCreateFile, (LPVOID*)&fpNtCreateFile) != MH_OK)
            {
                return FALSE;
            }

            if (MH_EnableHook((LPVOID)ntdllNtCreateFile) != MH_OK)
            {
                return FALSE;
            }


            // NtCreateNamedPipeFile
            tNtCreateFile ntdllNtCreateNamedPipeFile =
                (tNtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll"), "NtCreateNamedPipeFile");
            if (!ntdllNtCreateNamedPipeFile)
            {
                return FALSE;
            }

            if (MH_CreateHook((LPVOID)ntdllNtCreateNamedPipeFile, &DetourNtCreateNamedPipeFile,
                              (LPVOID*)&fpNtCreateNamedPipeFile) != MH_OK)
            {
                return FALSE;
            }

            if (MH_EnableHook((LPVOID)ntdllNtCreateNamedPipeFile) != MH_OK)
            {
                return FALSE;
            }


            // NtReadFile
            tNtReadFile ntdllNtReadFile = (tNtReadFile)GetProcAddress(GetModuleHandle(L"ntdll"), "NtReadFile");
            if (!ntdllNtReadFile)
            {
                return FALSE;
            }

            if (MH_CreateHook((LPVOID)ntdllNtReadFile, &DetourNtReadFile, (LPVOID*)&fpNtReadFile) != MH_OK)
            {
                return FALSE;
            }

            if (MH_EnableHook((LPVOID)ntdllNtReadFile) != MH_OK)
            {
                return FALSE;
            }

            // NtWriteFile
            tNtCreateFile ntdllNtWriteFile = (tNtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll"), "NtWriteFile");
            if (!ntdllNtWriteFile)
            {
                return FALSE;
            }

            if (MH_CreateHook((LPVOID)ntdllNtWriteFile, &DetourNtWriteFile, (LPVOID*)&fpNtWriteFile) != MH_OK)
            {
                return FALSE;
            }

            if (MH_EnableHook((LPVOID)ntdllNtWriteFile) != MH_OK)
            {
                return FALSE;
            }

            // NtFsControlFile
            tNtFsControlFile ntdllNtFsControlFile =
                (tNtFsControlFile)GetProcAddress(GetModuleHandle(L"ntdll"), "NtFsControlFile");
            if (!ntdllNtFsControlFile)
            {
                return FALSE;
            }

            if (MH_CreateHook((LPVOID)ntdllNtFsControlFile, &DetourNtFsControlFile, (LPVOID*)&fpNtFsControlFile) != MH_OK)
            {
                return FALSE;
            }

            if (MH_EnableHook((LPVOID)ntdllNtFsControlFile) != MH_OK)
            {
                return FALSE;
            }


            std::wstring pipeName = NAMED_PIPE_MONITOR + std::to_wstring(GetCurrentProcessId());
            std::unique_ptr<HANDLE, HandleDeleter> hThread(
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProxyPokedWorker, (LPVOID)pipeName.data(), 0, NULL));
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
