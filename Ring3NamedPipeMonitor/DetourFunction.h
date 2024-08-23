#ifndef DETOUR_FUNCTION
#define DETOUR_FUNCTION
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>

#define FSCTL_PIPE_LISTEN 0x110008

// NtCreateFile
typedef NTSTATUS(WINAPI* tNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                                        PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
                                        ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
                                        ULONG EaLength);
extern tNtCreateFile fpNtCreateFile;
extern tNtCreateFile ntdllNtCreateFile;
NTSTATUS WINAPI DetourNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                                   PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
                                   ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
                                   ULONG EaLength);

// NtCreateNamedPipeFile
typedef NTSTATUS(WINAPI* tNtCreateNamedPipeFile)(PHANDLE NamedPipeFileHandle, ULONG DesiredAccess,
                                                 POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
                                                 ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions,
                                                 ULONG NamedPipeType, ULONG ReadMode, ULONG CompletionMode,
                                                 ULONG MaximumInstances, ULONG InboundQuota, ULONG OutboundQuota,
                                                 PLARGE_INTEGER DefaultTimeout);
extern tNtCreateNamedPipeFile fpNtCreateNamedPipeFile;
extern tNtCreateNamedPipeFile ntdllNtCreateNamedPipeFile;
NTSTATUS WINAPI DetourNtCreateNamedPipeFile(PHANDLE NamedPipeFileHandle, ULONG DesiredAccess,
                                            POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
                                            ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions,
                                            ULONG NamedPipeType, ULONG ReadMode, ULONG CompletionMode,
                                            ULONG MaximumInstances, ULONG InboundQuota, ULONG OutboundQuota,
                                            PLARGE_INTEGER DefaultTimeout);

// NtReadFile
typedef NTSTATUS(WINAPI* tNtReadFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                      PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                      PULONG Key);
extern tNtReadFile fpNtReadFile;
extern tNtReadFile ntdllNtReadFile;
NTSTATUS WINAPI DetourNtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                 PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                 PULONG Key);


// NtWriteFile
typedef NTSTATUS(WINAPI* tNtWriteFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                       PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                       PULONG Key);
extern tNtWriteFile fpNtWriteFile;
extern tNtWriteFile ntdllNtWriteFile;
NTSTATUS WINAPI DetourNtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                  PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                  PULONG Key);

// NtFsControlFile
typedef NTSTATUS(WINAPI* tNtFsControlFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                           PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer,
                                           ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
extern tNtFsControlFile fpNtFsControlFile;
extern tNtFsControlFile ntdllNtFsControlFile;
NTSTATUS NTAPI DetourNtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
                                     PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer,
                                     ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
#endif
