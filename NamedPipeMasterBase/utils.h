#pragma once
#include <string>
#include <Windows.h>
#include <vector>

struct HandleDeleter
{
    typedef HANDLE pointer;
    void operator()(HANDLE handle)
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }
};

struct ScHandleDeleter
{
    typedef SC_HANDLE pointer;
    void operator()(SC_HANDLE handle)
    {
        if (handle)
            CloseServiceHandle(handle);
    }
};

struct MEMORY_INFO
{
    DWORD64 usingMemoryPercent;
    DWORD64 freeMemory;
    DWORD64 totalMemory;
};

VOID WriteStringToFile(const std::string& filename, const std::string& content);
std::string toUTF8(const std::wstring& input);
std::wstring toUTF16(const std::string& input);
std::wstring GetFileNameFromHandle(HANDLE hFile);
std::string GetNamedPipeName(HANDLE hPipe);
std::string toHexStr(DWORD64 num);
std::string toXxdFormat(const UCHAR* data, size_t length);
std::string GetProcessPathByPid(DWORD pid);
BOOL LoadDriver(const WCHAR* lpszDriverName, const WCHAR* lpszDriverPath, const WCHAR* lpszAltitude);
BOOL UnloadDriver(LPCWSTR driverName);
std::wstring GetCurrentDirPath();
std::wstring GetCurrentProcessPath();
BOOL GetMemoryInfo(MEMORY_INFO& systemMemInfo);
LONG WINAPI UefExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);
std::string GetCurrentTimeStr();
std::string Lower(const std::string& str);
std::string Upper(const std::string& str);
std::string GetModuleNameFromAddress(LPVOID address);
HMODULE GetModuleBaseAddress(LPVOID address);
std::vector<std::string> GetStackTrace();
std::string GetConcatStackTraceString();
BOOL EnableDebugPrivilege();
BOOL DllInject(std::string path, DWORD pid);
BOOL IsProcessElevated();
std::string isTrue(BOOL b);
