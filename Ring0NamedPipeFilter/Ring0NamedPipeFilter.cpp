#include "Ring0NamedPipeFilter.h"
#include <fltKernel.h>
#include "Ring0EventSender.h"
#include "../NamedPipeMasterBase/NamedPipeProtocol.h"

extern "C"
{
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NPMfUnload)
#pragma alloc_text(PAGE, NPMfInstanceQueryTeardown)
#pragma alloc_text(PAGE, NPMfInstanceSetup)
#pragma alloc_text(PAGE, NPMfPreCreateNamedPipe)
#pragma alloc_text(PAGE, NPMfPostCreateNamedPipe)
#pragma alloc_text(PAGE, NPMfPostReadWhenSafe)
#pragma alloc_text(PAGE, NPMfPostWriteWhenSafe)
#pragma alloc_text(PAGE, NPMfPostFSCtlWhenSafe)
#endif

    PFLT_FILTER gFilterHandle = nullptr;
    PDRIVER_OBJECT gDriverObject = nullptr;

    __declspec(allocate("INIT")) CONST FLT_OPERATION_REGISTRATION OperationCallbacks[] = {
        {IRP_MJ_CREATE, 0, NPMfPreCreateNamedPipe, NPMfPostCreateNamedPipe},
        {IRP_MJ_CREATE_NAMED_PIPE, 0, NPMfPreCreateNamedPipe, NPMfPostCreateNamedPipe},
        {IRP_MJ_READ, 0, nullptr, NPMfPostRead},
        {IRP_MJ_WRITE, 0, nullptr, NPMfPostWrite},
        {IRP_MJ_FILE_SYSTEM_CONTROL, 0, nullptr, NPMfPostFSCtl},
        {IRP_MJ_OPERATION_END}};

    __declspec(allocate("INIT")) CONST FLT_REGISTRATION FilterRegistration = {
        sizeof(FLT_REGISTRATION),                // Size
        FLT_REGISTRATION_VERSION,                // Version
        FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS,    // Flags
        nullptr,                                 // Context registration
        OperationCallbacks,                      // Operation callbacks
        NPMfUnload,                              // FilterUnload
        NPMfInstanceSetup,                       // InstanceSetup
        NPMfInstanceQueryTeardown,               // InstanceQueryTeardown
    };

    NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
    {
        UNREFERENCED_PARAMETER(RegistryPath);
        gDriverObject = DriverObject;

        NTSTATUS status = STATUS_SUCCESS;

        status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("FltRegisterFilter error: %X\n", status);
            goto _ERROR;
        }

        status = MinifltPortInitialize(gFilterHandle);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("MinifltPortInitialize error: %X\n", status);
            goto _ERROR;
        }

        status = FltStartFiltering(gFilterHandle);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("FltStartFiltering error: %X\n", status);
            goto _ERROR;
        }

        return status;

    _ERROR:
        MinifltPortFinalize();
        if (gFilterHandle)
        {
            FltUnregisterFilter(gFilterHandle);
        }

        return status;
    }

    NTSTATUS FLTAPI NPMfUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
    {
        UNREFERENCED_PARAMETER(Flags);
        PAGED_CODE();
        MinifltPortFinalize();
        FltUnregisterFilter(gFilterHandle);
        return STATUS_SUCCESS;
    }

    NTSTATUS FLTAPI NPMfInstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
                                      _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType)
    {
        UNREFERENCED_PARAMETER(FltObjects);
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(VolumeDeviceType);

        PAGED_CODE();

        NTSTATUS status = STATUS_SUCCESS;

        if (VolumeFilesystemType == FLT_FSTYPE_NPFS)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_FLT_DO_NOT_ATTACH;
        }

        return status;
    }

    NTSTATUS FLTAPI NPMfInstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                                              _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
    {
        UNREFERENCED_PARAMETER(FltObjects);
        UNREFERENCED_PARAMETER(Flags);

        PAGED_CODE();

        return STATUS_SUCCESS;
    }

    FLT_PREOP_CALLBACK_STATUS FLTAPI NPMfPreCreateNamedPipe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _Flt_CompletionContext_Outptr_ PVOID* CompletionContext)
    {
        UNREFERENCED_PARAMETER(FltObjects);

        PAGED_CODE();

        FLT_PREOP_CALLBACK_STATUS cbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

        // IRQL must <= APC_LEVEL to use ExAllocatePoolWithTag.
        if (KeGetCurrentIrql() > APC_LEVEL)
        {
            return cbStatus;
        }

        *CompletionContext = nullptr;
        PFLT_FILE_NAME_INFORMATION fileNameInfo = nullptr;

        // We pick up the name in pre-create for two reasons:
        // - If the create fails, the name cannot be queried in post-create.
        // - If the create succeeds, this is the cheapest place to pick it up since FileObject->FileName is valid.
        NTSTATUS status = STATUS_SUCCESS;
        __try
        {
            status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
                                               &fileNameInfo);
        }
        __finally
        {
            if (!NT_SUCCESS(status) || !fileNameInfo)
            {
                DbgPrint("FltGetFileNameInformation error: %X\n", status);
                return cbStatus;
            }
        }

        // The allocated memory for file name will be freed in the post operation.
        NPM_FILE_NAME_INFO* npmFileNameInfo =
            (NPM_FILE_NAME_INFO*)ExAllocatePoolWithTag(PagedPool, sizeof(NPM_FILE_NAME_INFO), 'NPMf');
        if (!npmFileNameInfo)
        {
            DbgPrint("ExAllocatePoolWithTag error\n");
            FltReleaseFileNameInformation(fileNameInfo);
            fileNameInfo = NULL;
            return cbStatus;
        }

        RtlZeroMemory(npmFileNameInfo, sizeof(NPM_FILE_NAME_INFO));
        npmFileNameInfo->fileNameLength =
            fileNameInfo->Name.Length > MAX_PIPE_NAME_LENGTH ? MAX_PIPE_NAME_LENGTH : fileNameInfo->Name.Length;
        RtlCopyMemory(npmFileNameInfo->fileName, fileNameInfo->Name.Buffer, npmFileNameInfo->fileNameLength);

        FltReleaseFileNameInformation(fileNameInfo);
        fileNameInfo = NULL;
        *CompletionContext = npmFileNameInfo;
        cbStatus = FLT_PREOP_SYNCHRONIZE;

        return cbStatus;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostCreateNamedPipe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                              _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                              _In_opt_ PVOID CompletionContext,
                                                              _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        UNREFERENCED_PARAMETER(Flags);

        PAGED_CODE();

        // Get file name from preoperation.
        NPM_FILE_NAME_INFO* npmFileNameInfo = (NPM_FILE_NAME_INFO*)CompletionContext;
        if (!npmFileNameInfo)
        {
            goto _EXIT;
        }

        // IRQL must <= APC_LEVEL to use ExAllocatePoolWithTag.
        if (KeGetCurrentIrql() > APC_LEVEL)
        {
            goto _EXIT;
        }

        // Exclude file names containing NamedPipeMaster and anonymous pipes.
        if (wcsstr(npmFileNameInfo->fileName, NAMED_PIPE_MASTER) != NULL ||
            !wcscmp(npmFileNameInfo->fileName, RING0_ANONYMOUS_PIPE))
        {
            goto _EXIT;
        }

        // Get thread id and process id.
        if (!MmIsAddressValid(Data->Thread))
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        ULONG threadId = HandleToUlong(PsGetThreadId(Data->Thread));
        ULONG processId = HandleToUlong(PsGetThreadProcessId(Data->Thread));

        // Check the file type is named pipe.
        auto& fileObject = FltObjects->FileObject;
        PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject(FltObjects->FileObject);
        if (!deviceObject || deviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
        {
            goto _EXIT;
        }

        switch (Data->Iopb->MajorFunction)
        {
            case IRP_MJ_CREATE:
            {
                ULONG desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
                ULONG fileAttributes = Data->Iopb->Parameters.Create.FileAttributes;
                ULONG shareAccess = Data->Iopb->Parameters.Create.ShareAccess;
                ULONG createDisposition = Data->Iopb->Parameters.Create.Options >> 24;
                ULONG createOptions = Data->Iopb->Parameters.Create.Options & 0xffffff;

                DbgPrint(
                    "CMD_CREATE threadId: %d, processId: %d, fileObject: 0x%08x, DesiredAccess: 0x%08x, "
                    "fileAttributes: 0x%08x, shareAccess: 0x%08x, createDisposition: 0x%08x, createOptions: %08x, fileName: "
                    "%ws\n",
                    threadId, processId, fileObject, desiredAccess, fileAttributes, shareAccess, createDisposition,
                    createOptions, npmFileNameInfo->fileName);

                ULONG pipeDataLength = sizeof(DATA_CREATE);
                DATA_CREATE* pipeData = (DATA_CREATE*)ExAllocatePoolWithTag(PagedPool, pipeDataLength, 'NPMf');
                if (!pipeData)
                {
                    DbgPrint("ExAllocatePoolWithTag error\n");
                    break;
                }
                RtlZeroMemory(pipeData, pipeDataLength);
                pipeData->cmdId = CMD_CREATE;
                pipeData->sourceType = SOURCE_MINIFILTER;
                pipeData->threadId = threadId;
                pipeData->processId = processId;
                pipeData->fileObject = (ULONG64)fileObject;
                pipeData->fileAttributes = fileAttributes;
                pipeData->shareAccess = shareAccess;
                pipeData->createDisposition = createDisposition;
                pipeData->createOptions = createOptions;
                pipeData->status = Data->IoStatus.Status;
                pipeData->fileNameLength = npmFileNameInfo->fileNameLength / 2;
                RtlCopyMemory(pipeData->fileName, npmFileNameInfo->fileName, npmFileNameInfo->fileNameLength);

                if (!NT_SUCCESS(SendEvent((PVOID)pipeData, pipeDataLength)))
                {
                    DbgPrint("SendEvent error");
                }

                ExFreePoolWithTag(pipeData, 'NPMf');
                pipeData = NULL;
                break;
            }
            case IRP_MJ_CREATE_NAMED_PIPE:
            {
                ULONG desiredAccess = Data->Iopb->Parameters.CreatePipe.SecurityContext->DesiredAccess;
                ULONG shareAccess = Data->Iopb->Parameters.CreatePipe.ShareAccess;
                ULONG createDisposition = Data->Iopb->Parameters.CreatePipe.Options >> 24;
                ULONG createOptions = Data->Iopb->Parameters.CreatePipe.Options & 0xffffff;
                PNAMED_PIPE_CREATE_PARAMETERS namedPipeCreateParameters =
                    static_cast<PNAMED_PIPE_CREATE_PARAMETERS>(Data->Iopb->Parameters.CreatePipe.Parameters);
                LONG namedPipeType = namedPipeCreateParameters->NamedPipeType;
                LONG readMode = namedPipeCreateParameters->ReadMode;
                LONG completionMode = namedPipeCreateParameters->CompletionMode;
                LONG maximumInstances = namedPipeCreateParameters->MaximumInstances;
                LONG inboundQuota = namedPipeCreateParameters->InboundQuota;
                LONG outboundQuota = namedPipeCreateParameters->OutboundQuota;
                LONG64 defaultTimeout = namedPipeCreateParameters->DefaultTimeout.QuadPart;

                DbgPrint(
                    "CMD_CREATE_NAMED_PIPE threadId: %d, processId: %d, fileObject: 0x%08x, desiredAccess: 0x%08x, "
                    "shareAccess: 0x%08x, createDisposition: 0x%08x, createOptions: 0x%08x, namedPipeType: 0x%08x, "
                    "readMode: 0x%08x, completionMode: 0x%08x, "
                    "maximumInstances: 0x%08x, inboundQuota: 0x%08x, outboundQuota: 0x%08x, defaultTimeout: 0x%08x, "
                    "fileName: %ws\n",
                    threadId, processId, fileObject, desiredAccess, shareAccess, createDisposition, createOptions,
                    namedPipeType, readMode, completionMode, maximumInstances, inboundQuota, outboundQuota, defaultTimeout,
                    npmFileNameInfo->fileName);

                ULONG pipeDataLength = sizeof(DATA_CREATE_NAMED_PIPE);
                DATA_CREATE_NAMED_PIPE* pipeData =
                    (DATA_CREATE_NAMED_PIPE*)ExAllocatePoolWithTag(PagedPool, pipeDataLength, 'NPMf');
                if (!pipeData)
                {
                    DbgPrint("ExAllocatePoolWithTag error\n");
                    break;
                }
                RtlZeroMemory(pipeData, pipeDataLength);
                pipeData->cmdId = CMD_CREATE_NAMED_PIPE;
                pipeData->sourceType = SOURCE_MINIFILTER;
                pipeData->threadId = threadId;
                pipeData->processId = processId;
                pipeData->fileObject = (ULONG64)fileObject;
                pipeData->desiredAccess = desiredAccess;
                pipeData->shareAccess = shareAccess;
                pipeData->createDisposition = createDisposition;
                pipeData->createOptions = createOptions;
                pipeData->namedPipeType = namedPipeType;
                pipeData->readMode = readMode;
                pipeData->completionMode = completionMode;
                pipeData->maximumInstances = maximumInstances;
                pipeData->inboundQuota = inboundQuota;
                pipeData->outboundQuota = outboundQuota;
                pipeData->defaultTimeout = defaultTimeout;
                pipeData->status = Data->IoStatus.Status;
                pipeData->fileNameLength = npmFileNameInfo->fileNameLength / 2;
                RtlCopyMemory(pipeData->fileName, npmFileNameInfo->fileName, npmFileNameInfo->fileNameLength);

                if (!NT_SUCCESS(SendEvent((PVOID)pipeData, pipeDataLength)))
                {
                    DbgPrint("SendEvent error");
                }

                ExFreePoolWithTag(pipeData, 'NPMf');
                pipeData = NULL;
                break;
            }
        }

    _EXIT:
        // Free the memory from preoperation.
        if (npmFileNameInfo)
        {
            ExFreePoolWithTag(npmFileNameInfo, 'NPMf');
            npmFileNameInfo = NULL;
        }

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostRead(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                   _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        FLT_POSTOP_CALLBACK_STATUS postOperationStatus = FLT_POSTOP_FINISHED_PROCESSING;

        if (NT_SUCCESS(Data->IoStatus.Status))
        {
            if (!FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, NPMfPostReadWhenSafe,
                                                   &postOperationStatus))
            {
            }
        }
        else
        {
        }

        return postOperationStatus;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostReadWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                           _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                           _In_opt_ PVOID CompletionContext,
                                                           _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        UNREFERENCED_PARAMETER(CompletionContext);
        UNREFERENCED_PARAMETER(Flags);

        PAGED_CODE();

        // IRQL must <= APC_LEVEL to use ExAllocatePoolWithTag.
        if (KeGetCurrentIrql() > APC_LEVEL)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        if (Data->IoStatus.Information == 0)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        NTSTATUS status = STATUS_SUCCESS;

        // Check the file type is named pipe.
        PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject(FltObjects->FileObject);
        if (!deviceObject || deviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        // Get file name with FltGetFileNameInformation.
        PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
        __try
        {
            status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
        }
        __finally
        {
            if (!NT_SUCCESS(status) || !fileNameInfo)
            {
                return FLT_POSTOP_FINISHED_PROCESSING;
            }
        }

        ULONG fileNameLength =
            fileNameInfo->Name.Length > MAX_PIPE_NAME_LENGTH ? MAX_PIPE_NAME_LENGTH : fileNameInfo->Name.Length;
        WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1] = {0};
        RtlCopyMemory(fileName, fileNameInfo->Name.Buffer, fileNameLength);

        FltReleaseFileNameInformation(fileNameInfo);
        fileNameInfo = NULL;

        // Exclude file names containing NamedPipeMaster and anonymous pipes.
        if (wcsstr(fileName, NAMED_PIPE_MASTER) != NULL || !wcscmp(fileName, RING0_ANONYMOUS_PIPE))
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        // Collect data to send to user mode.
        if (!MmIsAddressValid(Data->Thread))
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        ULONG threadId = HandleToUlong(PsGetThreadId(Data->Thread));
        ULONG processId = HandleToUlong(PsGetThreadProcessId(Data->Thread));
        ULONG readLength = (ULONG)Data->IoStatus.Information;
        ULONG copyReadLength = readLength > COPY_READ_WRITE_BUFFER_LENGTH ? COPY_READ_WRITE_BUFFER_LENGTH : readLength;
        ULONG pipeDataLength = sizeof(DATA_READ) + copyReadLength + 2;
        if (pipeDataLength > MAX_BUFFER_SIZE)
        {
            pipeDataLength = MAX_BUFFER_SIZE;
            copyReadLength = MAX_BUFFER_SIZE - sizeof(DATA_READ) - 2;
        }

        DbgPrint("CMD_READ FileObject: 0x%p, fileName: %ws, threadId: %d, processId %d, readLength: %d\n",
                 FltObjects->FileObject, fileName, threadId, processId, readLength);

        DATA_READ* pipeData = (DATA_READ*)ExAllocatePoolWithTag(PagedPool, pipeDataLength, 'NPMf');
        if (!pipeData)
        {
            DbgPrint("ExAllocatePoolWithTag error\n");
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        RtlZeroMemory(pipeData, pipeDataLength);
        pipeData->cmdId = CMD_READ;
        pipeData->sourceType = SOURCE_MINIFILTER;
        pipeData->threadId = threadId;
        pipeData->processId = processId;
        pipeData->fileObject = (ULONG64)FltObjects->FileObject;
        pipeData->status = Data->IoStatus.Status;
        pipeData->fileNameLength = fileNameLength / 2;
        RtlCopyMemory(pipeData->fileName, fileName, fileNameLength);
        pipeData->readLength = readLength;

        // To get read buffer, we need to use FltLockUserBuffer, then we must call MmGetSystemAddressForMdlSafe.
        PUCHAR readBuffer = NULL;
        __try
        {
            status = FltLockUserBuffer(Data);
            if (NT_SUCCESS(status))
            {
                readBuffer = (PUCHAR)(MmGetSystemAddressForMdlSafe(
                    Data->Iopb->Parameters.Read.MdlAddress, LowPagePriority | MdlMappingNoWrite | MdlMappingNoExecute));
            }
        }
        __finally
        {
            if (readBuffer)
            {
                RtlCopyMemory(pipeData->readBuffer, readBuffer, copyReadLength);
                MmUnmapLockedPages(readBuffer, Data->Iopb->Parameters.Read.MdlAddress);
            }
        }

        if (!NT_SUCCESS(SendEvent((PVOID)pipeData, pipeDataLength)))
        {
            DbgPrint("SendEvent error");
        }

        ExFreePoolWithTag(pipeData, 'NPMf');
        pipeData = NULL;

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostWrite(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                    _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        FLT_POSTOP_CALLBACK_STATUS postOperationStatus = FLT_POSTOP_FINISHED_PROCESSING;

        if (NT_SUCCESS(Data->IoStatus.Status))
        {
            if (!FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, NPMfPostWriteWhenSafe,
                                                   &postOperationStatus))
            {
            }
        }
        else
        {
        }

        return postOperationStatus;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostWriteWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _In_opt_ PVOID CompletionContext,
                                                            _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        UNREFERENCED_PARAMETER(CompletionContext);
        UNREFERENCED_PARAMETER(Flags);

        PAGED_CODE();

        // IRQL must <= APC_LEVEL to use ExAllocatePoolWithTag.
        if (KeGetCurrentIrql() > APC_LEVEL)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        if (Data->IoStatus.Information == 0)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        NTSTATUS status = STATUS_SUCCESS;

        // Check the file type is named pipe.
        PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject(FltObjects->FileObject);
        if (!deviceObject || deviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        // Get file name with FltGetFileNameInformation.
        PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
        __try
        {
            status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
        }
        __finally
        {
            if (!NT_SUCCESS(status) || !fileNameInfo)
            {
                return FLT_POSTOP_FINISHED_PROCESSING;
            }
        }

        ULONG fileNameLength =
            fileNameInfo->Name.Length > MAX_PIPE_NAME_LENGTH ? MAX_PIPE_NAME_LENGTH : fileNameInfo->Name.Length;
        WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1] = {0};
        RtlCopyMemory(fileName, fileNameInfo->Name.Buffer, fileNameLength);

        FltReleaseFileNameInformation(fileNameInfo);
        fileNameInfo = NULL;

        // Exclude file names containing NamedPipeMaster and anonymous pipes.
        if (wcsstr(fileName, NAMED_PIPE_MASTER) != NULL || !wcscmp(fileName, RING0_ANONYMOUS_PIPE))
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        // Collect data to send to user mode.
        if (!MmIsAddressValid(Data->Thread))
        {
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        ULONG threadId = HandleToUlong(PsGetThreadId(Data->Thread));
        ULONG processId = HandleToUlong(PsGetThreadProcessId(Data->Thread));
        ULONG writeLength = (ULONG)Data->IoStatus.Information;
        ULONG copyWriteLength = writeLength > COPY_READ_WRITE_BUFFER_LENGTH ? COPY_READ_WRITE_BUFFER_LENGTH : writeLength;
        ULONG pipeDataLength = sizeof(DATA_WRITE) + copyWriteLength + 2;
        if (pipeDataLength > MAX_BUFFER_SIZE)
        {
            pipeDataLength = MAX_BUFFER_SIZE;
            copyWriteLength = MAX_BUFFER_SIZE - sizeof(DATA_READ) - 2;
        }

        DbgPrint("CMD_WRITE FileObject: 0x%p, fileName: %ws, threadId: %d, processId %d, writeLength: %d\n",
                 FltObjects->FileObject, fileName, threadId, processId, writeLength);

        DATA_WRITE* pipeData = (DATA_WRITE*)ExAllocatePoolWithTag(PagedPool, pipeDataLength, 'NPMf');
        if (!pipeData)
        {
            DbgPrint("ExAllocatePoolWithTag error\n");
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        RtlZeroMemory(pipeData, pipeDataLength);
        pipeData->cmdId = CMD_WRITE;
        pipeData->sourceType = SOURCE_MINIFILTER;
        pipeData->threadId = threadId;
        pipeData->processId = processId;
        pipeData->fileObject = (ULONG64)FltObjects->FileObject;
        pipeData->status = Data->IoStatus.Status;
        pipeData->fileNameLength = fileNameLength / 2;
        RtlCopyMemory(pipeData->fileName, fileName, fileNameLength);
        pipeData->writeLength = writeLength;

        // To get write buffer, we need to use FltLockUserBuffer, then we must call MmGetSystemAddressForMdlSafe.
        PUCHAR writeBuffer = NULL;
        __try
        {
            status = FltLockUserBuffer(Data);
            if (NT_SUCCESS(status))
            {
                writeBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(
                    Data->Iopb->Parameters.Write.MdlAddress, LowPagePriority | MdlMappingNoWrite | MdlMappingNoExecute);
            }
        }
        __finally
        {
            if (writeBuffer)
            {
                RtlCopyMemory(pipeData->writeBuffer, writeBuffer, copyWriteLength);
                MmUnmapLockedPages(writeBuffer, Data->Iopb->Parameters.Write.MdlAddress);
            }
        }

        if (!NT_SUCCESS(SendEvent((PVOID)pipeData, pipeDataLength)))
        {
            DbgPrint("SendEvent error");
        }

        ExFreePoolWithTag(pipeData, 'NPMf');
        pipeData = NULL;

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostFSCtl(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                    _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        FLT_POSTOP_CALLBACK_STATUS postOperationStatus = FLT_POSTOP_FINISHED_PROCESSING;

        if (!FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, NPMfPostFSCtlWhenSafe,
                                               &postOperationStatus))
        {
        }

        return postOperationStatus;
    }

    FLT_POSTOP_CALLBACK_STATUS FLTAPI NPMfPostFSCtlWhenSafe(_Inout_ PFLT_CALLBACK_DATA Data,
                                                            _In_ PCFLT_RELATED_OBJECTS FltObjects,
                                                            _In_opt_ PVOID CompletionContext,
                                                            _In_ FLT_POST_OPERATION_FLAGS Flags)
    {
        PAGED_CODE();
        UNREFERENCED_PARAMETER(FltObjects);
        UNREFERENCED_PARAMETER(CompletionContext);
        UNREFERENCED_PARAMETER(Flags);

        __try
        {
            ULONG& fsctl = Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode;
            switch (fsctl)
            {
                case FSCTL_PIPE_ASSIGN_EVENT:
                    // DbgPrint("FSCTL_PIPE_ASSIGN_EVENT Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_DISCONNECT:
                    // DbgPrint("FSCTL_PIPE_DISCONNECT Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_LISTEN:
                {
                    // IRQL must <= APC_LEVEL to use ExAllocatePoolWithTag.
                    if (KeGetCurrentIrql() > APC_LEVEL)
                    {
                        return FLT_POSTOP_FINISHED_PROCESSING;
                    }

                    NTSTATUS status = STATUS_SUCCESS;

                    // Check the file type is named pipe.
                    PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject(FltObjects->FileObject);
                    if (!deviceObject || deviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
                    {
                        return FLT_POSTOP_FINISHED_PROCESSING;
                    }

                    // Get file name with FltGetFileNameInformation.
                    PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
                    __try
                    {
                        status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                           &fileNameInfo);
                    }
                    __finally
                    {
                        if (!NT_SUCCESS(status) || !fileNameInfo)
                        {
                            return FLT_POSTOP_FINISHED_PROCESSING;
                        }
                    }

                    ULONG fileNameLength =
                        fileNameInfo->Name.Length > MAX_PIPE_NAME_LENGTH ? MAX_PIPE_NAME_LENGTH : fileNameInfo->Name.Length;
                    WCHAR fileName[MAX_PIPE_NAME_LENGTH + 1] = {0};
                    RtlCopyMemory(fileName, fileNameInfo->Name.Buffer, fileNameLength);

                    FltReleaseFileNameInformation(fileNameInfo);
                    fileNameInfo = NULL;

                    // Exclude file names containing NamedPipeMaster and anonymous pipes.
                    if (wcsstr(fileName, NAMED_PIPE_MASTER) != NULL || !wcscmp(fileName, RING0_ANONYMOUS_PIPE))
                    {
                        return FLT_POSTOP_FINISHED_PROCESSING;
                    }


                    // Collect data to send to user mode.
                    if (!MmIsAddressValid(Data->Thread))
                    {
                        return FLT_POSTOP_FINISHED_PROCESSING;
                    }
                    ULONG threadId = HandleToUlong(PsGetThreadId(Data->Thread));
                    ULONG processId = HandleToUlong(PsGetThreadProcessId(Data->Thread));

                    DbgPrint("CMD_CONNECT_NAMED_PIPE FileObject: 0x%p, fileName: %ws, threadId: %d, processId %d\n",
                             FltObjects->FileObject, fileName, threadId, processId);

                    ULONG pipeDataLength = sizeof(DATA_CONNECT_NAMED_PIPE);
                    DATA_CONNECT_NAMED_PIPE* pipeData =
                        (DATA_CONNECT_NAMED_PIPE*)ExAllocatePoolWithTag(PagedPool, pipeDataLength, 'NPMf');
                    if (!pipeData)
                    {
                        DbgPrint("ExAllocatePoolWithTag error\n");
                        return FLT_POSTOP_FINISHED_PROCESSING;
                    }

                    RtlZeroMemory(pipeData, pipeDataLength);
                    pipeData->cmdId = CMD_CONNECT_NAMED_PIPE;
                    pipeData->sourceType = SOURCE_MINIFILTER;
                    pipeData->threadId = threadId;
                    pipeData->processId = processId;
                    pipeData->fileObject = (ULONG64)FltObjects->FileObject;
                    pipeData->status = Data->IoStatus.Status;
                    pipeData->fileNameLength = fileNameLength / 2;
                    RtlCopyMemory(pipeData->fileName, fileName, fileNameLength);

                    if (!NT_SUCCESS(SendEvent((PVOID)pipeData, pipeDataLength)))
                    {
                        DbgPrint("SendEvent error");
                    }

                    ExFreePoolWithTag(pipeData, 'NPMf');
                    pipeData = NULL;

                    break;
                }
                break;
                case FSCTL_PIPE_PEEK:
                    // DbgPrint("FSCTL_PIPE_PEEK Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %!STATUS!", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_QUERY_EVENT:
                    // DbgPrint("FSCTL_PIPE_QUERY_EVENT Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_TRANSCEIVE:
                    // DbgPrint("FSCTL_PIPE_TRANSCEIVE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_WAIT:
                    // DbgPrint("FSCTL_PIPE_WAIT Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    // FltObjects->FileObject,
                    //         FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_IMPERSONATE:
                    // DbgPrint("FSCTL_PIPE_IMPERSONATE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_SET_CLIENT_PROCESS:
                    // DbgPrint("FSCTL_PIPE_SET_CLIENT_PROCESS Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
                    // DbgPrint("FSCTL_PIPE_QUERY_CLIENT_PROCESS Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_GET_PIPE_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_GET_PIPE_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_SET_PIPE_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_SET_PIPE_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_GET_HANDLE_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_GET_HANDLE_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_SET_HANDLE_ATTRIBUTE:
                    // DbgPrint("FSCTL_PIPE_SET_HANDLE_ATTRIBUTE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_FLUSH:
                    // DbgPrint("FSCTL_PIPE_FLUSH Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_INTERNAL_READ:
                    // DbgPrint("FSCTL_PIPE_INTERNAL_READ Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_INTERNAL_WRITE:
                    // DbgPrint("FSCTL_PIPE_INTERNAL_WRITE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
                    // DbgPrint("FSCTL_PIPE_TRANSCEIVE Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
                    // DbgPrint("FSCTL_PIPE_READ_OVFLOW Cbd 0x%p FileObject 0x%p FileKey 0x%p Status %d", Data,
                    //         FltObjects->FileObject, FltObjects->FileObject->FsContext, Data->IoStatus.Status);
                    break;
                default:
                    // DbgPrint("Unknown FSCTL 0x%08lx", fsctl);
                    break;
            }
        }
        __finally
        {
        }

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

}    // extern "C"
