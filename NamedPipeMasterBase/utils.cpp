#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <cstdio>
#include <string>
#include <SetupAPI.h>
#include <winternl.h>
#include "../NamedPipeMasterBase/NPMLogger.h"

#define MAX_DISPLAY_MEMORY 0x100

void WriteStringToFile(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename, std::ios_base::app);
    if (file.is_open())
    {
        file << content << std::endl;
        file.close();
    }
}

std::string toUTF8(const std::wstring &input)
{
    int length = WideCharToMultiByte(CP_UTF8, NULL, input.c_str(), (int)input.size(), NULL, 0, NULL, NULL);
    if (!(length > 0))
        return std::string();
    else
    {
        std::string result;
        result.resize(length);

        if (WideCharToMultiByte(CP_UTF8, NULL, input.c_str(), (int)input.size(), &result[0], result.size(), NULL, NULL) > 0)
            return result;
        else
            return "";
    }
}

std::wstring toUTF16(const std::string &input)
{
    int length = MultiByteToWideChar(CP_UTF8, NULL, input.c_str(), (int)input.size(), NULL, 0);
    if (!(length > 0))
        return std::wstring();
    else
    {
        std::wstring result;
        result.resize(length);

        if (MultiByteToWideChar(CP_UTF8, NULL, input.c_str(), (int)input.size(), &result[0], result.size()) > 0)
            return result;
        else
            return L"";
    }
}

std::string toHexStr(DWORD64 num)
{
    std::stringstream stream;
    stream << std::hex << num;
    std::string s(stream.str());
    return s;
}

std::wstring GetFileNameFromHandle(HANDLE hFile)
{
    std::unique_ptr<BYTE[]> ptrcFni(new BYTE[MAX_PATH * sizeof(TCHAR) + sizeof(FILE_NAME_INFO)]);
    FILE_NAME_INFO *pFni = reinterpret_cast<FILE_NAME_INFO *>(ptrcFni.get());

    if (!GetFileInformationByHandleEx(hFile, FileNameInfo, pFni, sizeof(FILE_NAME_INFO) + (MAX_PATH * sizeof(TCHAR))))
    {
        return L"";
    }

    return std::wstring(pFni->FileName, pFni->FileNameLength / sizeof(TCHAR));
}

std::string toXxdFormat(const UCHAR *data, size_t length)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (size_t i = 0; i < length; i += 16)
    {
        // memory offset
        ss << std::setw(8) << i << ": ";

        // Set char in hex.
        for (size_t j = i; j < i + 16; ++j)
        {
            if (j < length)
            {
                ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(data[j])) << " ";
            }
            else
            {
                // Pad with space for the left characters.
                ss << "   ";
            }
        }

        // Store it as ascii or dot.
        ss << " ";
        for (size_t j = i; j < i + 16 && j < length; ++j)
        {
            if (isprint(data[j]))
            {
                ss << data[j];
            }
            else
            {
                ss << ".";
            }
        }

        ss << "\n";
    }

    return ss.str();
}

std::string GetNamedPipeName(HANDLE hPipe)
{
    WCHAR pipeName[MAX_PATH + 1];

    if (!GetNamedPipeInfo(hPipe, NULL, NULL, NULL, NULL))
    {
        std::cerr << "GetNamedPipeInfo failed, error " << GetLastError() << std::endl;
        return "";
    }

    if (!GetNamedPipeHandleState(hPipe, NULL, NULL, NULL, NULL, pipeName, MAX_PATH))
    {
        std::cerr << "GetNamedPipeHandleState failed, error " << GetLastError() << std::endl;
        return "";
    }

    return toUTF8(std::wstring(pipeName));
}

std::string GetProcessPathByPid(DWORD pid)
{
    std::string process_path = "";
    typedef BOOL(WINAPI * QueryFullProcessImageNamePROC)(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize);
    static QueryFullProcessImageNamePROC pQueryFullProcessImageNameA =
        (QueryFullProcessImageNamePROC)GetProcAddress(LoadLibrary(L"kernel32.dll"), "QueryFullProcessImageNameA");
    HANDLE phandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid);
    if (phandle != NULL)
    {
        const DWORD path_size = MAX_PATH * 2;
        CHAR path[path_size];
        ZeroMemory(path, sizeof(path));
        if (pQueryFullProcessImageNameA)
        {
            DWORD size = path_size;
            if (pQueryFullProcessImageNameA(phandle, 0, path, &size))
            {
                process_path = path;
            }
        }
        else
        {
            GetProcessImageFileNameA(phandle, path, path_size);
        }
        CloseHandle(phandle);
    }

    return process_path;
}

BOOL LoadDriver(const WCHAR *lpszDriverName, const WCHAR *lpszDriverPath, const WCHAR *lpszAltitude)
{
    // Open SCM(Service Control Manager).
    std::unique_ptr<SC_HANDLE, ScHandleDeleter> hSCManager(OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE));
    if (hSCManager == NULL)
    {
        return FALSE;
    }

    // Create a service for filter driver.
    std::unique_ptr<SC_HANDLE, ScHandleDeleter> hService(
        CreateService(hSCManager.get(), lpszDriverName, lpszDriverName, SERVICE_ALL_ACCESS, SERVICE_FILE_SYSTEM_DRIVER,
                      SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, lpszDriverPath, L"FSFilter Activity Monitor", NULL,
                      L"FltMgr\0", NULL, NULL));
    if (!hService)
    {
        // Open if the driver already exists.
        hService.reset(OpenService(hSCManager.get(), lpszDriverName, SERVICE_ALL_ACCESS));

        if (!hService.get())
        {
            return FALSE;
        }
    }

    NTSTATUS status;
    HKEY hKey = NULL;
    WCHAR szTempStr[MAX_PATH];
    ZeroMemory(szTempStr, sizeof(szTempStr));
    DWORD dwData = NULL;

    // Fill in the subkey of SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances
    wcscpy_s(szTempStr, L"SYSTEM\\CurrentControlSet\\Services\\");
    wcscat_s(szTempStr, lpszDriverName);
    wcscat_s(szTempStr, L"\\Instances");
    status =
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTempStr, 0, (LPWSTR)L"", TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData);
    if (status != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set DefaultInstance.
    wcscpy_s(szTempStr, lpszDriverName);
    wcscat_s(szTempStr, L" Instance");
    status = RegSetValueEx(hKey, L"DefaultInstance", 0, REG_SZ, (CONST BYTE *)szTempStr, (DWORD)wcslen(szTempStr) * 2);

    if (status != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegFlushKey(hKey);
    RegCloseKey(hKey);

    // Fill in the subkey of SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances\\DriverName\\Instance
    wcscpy_s(szTempStr, L"SYSTEM\\CurrentControlSet\\Services\\");
    wcscat_s(szTempStr, lpszDriverName);
    wcscat_s(szTempStr, L"\\Instances\\");
    wcscat_s(szTempStr, lpszDriverName);
    wcscat_s(szTempStr, L" Instance");

    status =
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTempStr, 0, (LPWSTR)L"", TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData);
    if (status != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set Altitude.
    wcscpy_s(szTempStr, lpszAltitude);
    status = RegSetValueEx(hKey, L"Altitude", 0, REG_SZ, (CONST BYTE *)szTempStr, (DWORD)wcslen(szTempStr) * 2);
    if (status != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set flags.
    dwData = 0x0;
    status = RegSetValueEx(hKey, L"Flags", 0, REG_DWORD, (CONST BYTE *)&dwData, sizeof(DWORD));
    if (status != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegFlushKey(hKey);
    RegCloseKey(hKey);

    // Start filter driver.
    LPCWSTR *arguments = NULL;
    if (StartService(hService.get(), 0, arguments) == FALSE && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL UnloadDriver(LPCWSTR driverName)
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
        return FALSE;

    SC_HANDLE hService = OpenService(hSCManager, driverName, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    SERVICE_STATUS service_status;
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &service_status))
    {
        return FALSE;
    }

    if (!DeleteService(hService))
    {
        return FALSE;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return TRUE;
}

std::wstring GetCurrentDirPath()
{
    const DWORD MAX_PATH_SIZE = 300;
    WCHAR strPath[MAX_PATH_SIZE] = {0};
    GetModuleFileName(NULL, strPath, MAX_PATH_SIZE);
    for (size_t j = wcslen(strPath); strPath[j] != L'\\'; j--)
    {
        strPath[j] = '\0';
    }
    return strPath;
}

std::wstring GetCurrentProcessPath()
{
    wchar_t file_path[MAX_PATH];
    ZeroMemory(file_path, sizeof(file_path));
    GetModuleFileName(NULL, file_path, MAX_PATH);
    return file_path;
}

DWORD GetProcessUsingMemory(HANDLE pHandle)
{
    PROCESS_MEMORY_COUNTERS_EX pmc;

    if (GetProcessMemoryInfo(pHandle, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        return (DWORD)(pmc.PrivateUsage / 1024 / 1024);
    }
    return NULL;
}

BOOL GetMemoryInfo(MEMORY_INFO &systemMemInfo)
{
    MEMORYSTATUSEX memoryStatus = {0};
    memoryStatus.dwLength = sizeof(memoryStatus);
    systemMemInfo.freeMemory = NULL;
    systemMemInfo.usingMemoryPercent = NULL;
    systemMemInfo.totalMemory = NULL;
    BOOL bRet = GlobalMemoryStatusEx(&memoryStatus);
    if (bRet)
    {
        systemMemInfo.usingMemoryPercent = memoryStatus.dwMemoryLoad;
        systemMemInfo.totalMemory = memoryStatus.ullTotalPhys / (1024 * 1024);
        systemMemInfo.freeMemory = memoryStatus.ullAvailPhys / (1024 * 1024);
        return TRUE;
    }
    return FALSE;
}

LONG WINAPI UefExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    // Log process and system information when encountering unhandled exception.
    NPMLogger::LoggerInstance()->error("[{}] unhandled exception", __FUNCTION__);

    MEMORY_INFO systemMemInfo;
    if (GetMemoryInfo(systemMemInfo))
    {
        NPMLogger::LoggerInstance()->info("system total memory: {} MB, system free memory: {} MB, using {}%",
                                          systemMemInfo.totalMemory, systemMemInfo.freeMemory,
                                          systemMemInfo.usingMemoryPercent);
    }

    DWORD processUsingMemory = GetProcessUsingMemory(GetCurrentProcess());
    NPMLogger::LoggerInstance()->info("process using memory: {} MB", processUsingMemory);

    NPMLogger::LoggerInstance()->info("stack trace:\n{}", GetConcatStackTraceString());

    return EXCEPTION_EXECUTE_HANDLER;
}

std::string GetCurrentTimeStr()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
    localtime_s(&local_tm, &now_time_t);
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Lower(const std::string &str)
{
    std::string lower_str = str;
    if (str.size())
    {
        transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::towlower);
    }
    return lower_str;
}

std::string Upper(const std::string &str)
{
    std::string upper_str = str;
    if (str.size())
    {
        transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::towupper);
    }
    return upper_str;
}

std::string GetModuleNameFromAddress(LPVOID address)
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCTSTR>(address), &hModule);
    WCHAR moduleFileName[MAX_PATH];
    GetModuleFileName(hModule, moduleFileName, MAX_PATH);
    return toUTF8(moduleFileName);
}

HMODULE GetModuleBaseAddress(LPVOID address)
{
    HMODULE moduleHandle = NULL;
    DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (GetModuleHandleEx(flags, reinterpret_cast<LPCWSTR>(address), &moduleHandle))
    {
        return moduleHandle;
    }
    return NULL;
}

std::vector<std::string> GetStackTrace()
{
    std::vector<std::string> stackTrace;

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
    {
        return stackTrace;
    }

    const DWORD maxFrames = 64;
    PVOID stackFrames[maxFrames];
    USHORT frames = CaptureStackBackTrace(0, maxFrames, stackFrames, NULL);

    for (USHORT i = 0; i < frames; i++)
    {
        std::string stackFrame = toHexStr((DWORD64)stackFrames[i] - (DWORD64)GetModuleBaseAddress(stackFrames[i]));
        std::string moduleName = GetModuleNameFromAddress(stackFrames[i]);
        std::string symbolName = "";

        DWORD64 displacement = 0;
        TCHAR buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        if (SymFromAddr(GetCurrentProcess(), (DWORD64)stackFrames[i], &displacement, symbol))
        {
            symbolName = "(" + std::string(symbol->Name) + " + 0x" + toHexStr(displacement) + ")";
        }
        else
        {
            symbolName = "(Unknown symbol)";
        }

        stackTrace.push_back("#" + std::to_string(i + 1) + ": " + moduleName + " + 0x" + stackFrame + " " + symbolName);
    }

    SymCleanup(GetCurrentProcess());

    return stackTrace;
}

std::string GetConcatStackTraceString()
{
    std::vector<std::string> vStackTrace = GetStackTrace();
    std::string sStackTrace = "";
    for (std::string &st : vStackTrace)
    {
        sStackTrace += st + " ->\n";
    }

    sStackTrace = sStackTrace.substr(0, sStackTrace.size() - 4);
    sStackTrace += "\n";

    return sStackTrace;
}

BOOL EnableDebugPrivilege()
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return FALSE;
    }
    std::unique_ptr<HANDLE, HandleDeleter> upToken(hToken);

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        return FALSE;
    }

    TOKEN_PRIVILEGES tokenPrivileges;
    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Luid = luid;
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(upToken.get(), FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL DllInject(std::string path, DWORD pid)
{
    std::unique_ptr<HANDLE, HandleDeleter> hprocess(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));

    if (hprocess.get() == NULL)
    {
        std::cout << "OpenProcess error: " << GetLastError() << std::endl;
        return FALSE;
    }

    PVOID procdlladdr = VirtualAllocEx(hprocess.get(), NULL, path.size(), MEM_COMMIT, PAGE_READWRITE);
    if (procdlladdr == NULL)
    {
        std::cout << "VirtualAllocEx error: " << GetLastError() << std::endl;
        return FALSE;
    }

    SIZE_T writenum;
    if (!WriteProcessMemory(hprocess.get(), procdlladdr, path.data(), path.size(), &writenum))
    {
        std::cout << "WriteProcessMemory error: " << GetLastError() << std::endl;
        return FALSE;
    }

    FARPROC loadfuncaddr = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
    if (!loadfuncaddr)
    {
        std::cout << "GetProcAddress error: " << GetLastError() << std::endl;
        return FALSE;
    }

    std::unique_ptr<HANDLE, HandleDeleter> hthread(
        CreateRemoteThread(hprocess.get(), NULL, 0, (LPTHREAD_START_ROUTINE)loadfuncaddr, (LPVOID)procdlladdr, 0, NULL));
    if (!hthread)
    {
        std::cout << "CreateRemoteThread error: " << GetLastError() << std::endl;
        return FALSE;
    }

    return TRUE;
}

BOOL IsProcessElevated()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        return FALSE;
    }
    std::unique_ptr<HANDLE, HandleDeleter> upToken(hToken);

    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (!GetTokenInformation(upToken.get(), TokenElevation, &elevation, sizeof(elevation), &dwSize))
    {
        return FALSE;
    }

    BOOL fIsElevated = elevation.TokenIsElevated;

    return fIsElevated;
}

std::string isTrue(BOOL b)
{
    if (b)
    {
        return "yes";
    }

    return "no";
}
