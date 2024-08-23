#pragma once
#define FILTER_DRIVER_NAME L"Ring0NamedPipeFilter"
#define FILTER_DRIVER_ALTITUDE L"370030"
#define NAMED_PIPE_MASTER L"NamedPipeMaster"
#define RING0_ANONYMOUS_PIPE L"\\Device\\NamedPipe\0"
#define RING3_ANONYMOUS_PIPE L"\\\\.\\pipe\\"
#define NAMED_PIPE_MASTER_PORT_NAME L"\\NamedPipeMaster"
#define MAX_PIPE_NAME_LENGTH 256
#define MAX_BUFFER_SIZE 0x1000
#define INJECTED_DLL_NAME L"Ring3NamedPipeMonitor"
#define COPY_READ_WRITE_BUFFER_LENGTH 1024
#define NAMED_PIPE_MONITOR L"NamedPipeMonitor"
