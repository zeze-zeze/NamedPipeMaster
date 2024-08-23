#include "NamedPipeEventDatabase.h"
#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include <algorithm>
#include <map>
#include "NamedPipeProtocol.h"
#include "NPMLogger.h"
#include "utils.h"

using namespace sqlite_orm;

std::map<CMD_ID, std::string> cmdId2str = {{CMD_CREATE_NAMED_PIPE, "CMD_CREATE_NAMED_PIPE"},
                                           {CMD_CREATE, "CMD_CREATE"},
                                           {CMD_READ, "CMD_READ"},
                                           {CMD_WRITE, "CMD_WRITE"},
                                           {CMD_CONNECT_NAMED_PIPE, "CMD_CONNECT_NAMED_PIPE"}};

std::map<SOURCE_TYPE, std::string> sourceType2str = {{SOURCE_MINIFILTER, "SOURCE_MINIFILTER"},
                                                     {SOURCE_INJECTED_DLL, "SOURCE_INJECTED_DLL"}};

BOOL InsertDatabaseListener();
BOOL RegularPrintDatabase();

BOOL InsertDatabaseListener()
{
    while (TRUE)
    {
        // If there is no named pipe event yet, wait for 200 miliseconds.
        if (NamedPipeEventDatabase::Instance().GetQueueSize() == 0)
        {
            Sleep(200);
            continue;
        }

        // Get named pipe event from queue.
        NAMED_PIPE_EVENT namedPipeEvent = NamedPipeEventDatabase::Instance().PopEvent();

        // Insert named pipe event into database.
        NamedPipeEventDatabase::Instance().InsertDatabase(namedPipeEvent);

        // Rotate database if the size is 90% full.
        if (NamedPipeEventDatabase::Instance().GetDatabaseSize() >
            NamedPipeEventDatabase::Instance().GetMaxDbSize() / 10 * 9)
        {
            NamedPipeEventDatabase::Instance().RotateDatabase();
        }
    }

    return TRUE;
}

BOOL RegularPrintDatabase()
{
    while (TRUE)
    {
        std::vector<NAMED_PIPE_EVENT> namedPipeEvents =
            NamedPipeEventDatabase::Instance().SelectDatabase(NamedPipeEventDatabase::Instance().GetHighestPrintedId());

        if (!namedPipeEvents.empty())
        {
            for (NAMED_PIPE_EVENT& namedPipeEvent : namedPipeEvents)
            {
                NamedPipeEventDatabase::Instance().PrintEvent(namedPipeEvent,
                                                              NamedPipeEventDatabase::Instance().GetMonitorMode());
            }
            NamedPipeEventDatabase::Instance().SetHighestPrintedId(namedPipeEvents.back().id + 1);
        }

        Sleep(500);
    }

    return TRUE;
}

NamedPipeEventDatabase::NamedPipeEventDatabase()
{
    monitorMode = FALSE;
    storage = std::make_unique<NamedPipeEventStorage>(InitStorage());
    storage->sync_schema();
    storage->remove_all<NAMED_PIPE_EVENT>();
}

BOOL NamedPipeEventDatabase::InitializeDatabase()
{
    ResetFilter();

    std::unique_ptr<HANDLE, HandleDeleter> hInsertDatabaseListenerThread(
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InsertDatabaseListener, NULL, 0, NULL));
    if (hInsertDatabaseListenerThread.get() == NULL)
    {
        NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
        return FALSE;
    }

    highestPrintedId = 0;
    std::unique_ptr<HANDLE, HandleDeleter> hRegularPrintDatabaseThread(
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RegularPrintDatabase, NULL, 0, NULL));
    if (hRegularPrintDatabaseThread.get() == NULL)
    {
        NPMLogger::LoggerInstance()->error("[{}] CreateThread error: {}", __FUNCTION__, GetLastError());
        return FALSE;
    }

    return TRUE;
}

VOID NamedPipeEventDatabase::SetMaxDbSize(DWORD _maxDbSize)
{
    maxDbSize = _maxDbSize;
}

DWORD NamedPipeEventDatabase::GetMaxDbSize()
{
    return maxDbSize;
}

VOID NamedPipeEventDatabase::SetMonitorMode(BOOL _monitorMode)
{
    std::lock_guard<std::mutex> lock(monitorMutex);
    monitorMode = _monitorMode;
}

BOOL NamedPipeEventDatabase::GetMonitorMode()
{
    std::lock_guard<std::mutex> lock(monitorMutex);
    return monitorMode;
}

BOOL NamedPipeEventDatabase::PrintCreateNamedPipeEvent(const NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::string stackTrace = "";
    if (!namedPipeEvent.stackTrace.empty())
    {
        stackTrace = "stackTrace: \n" + namedPipeEvent.stackTrace;
    }

    NPMLogger::LoggerInstance()->info(
        "\n"
        "cmdId: {}\n"
        "sourceType: {}\n"
        "threadId: {}\n"
        "processId: {}\n"
        "imagePath: {}\n"
        "fileObject: {}\n"
        "desiredAccess: {}\n"
        "fileAttributes: {}\n"
        "shareAccess: {}\n"
        "createDisposition: {}\n"
        "createOptions: {}\n"
        "namedPipeType: {}\n"
        "readMode: {}\n"
        "completionMode: {}\n"
        "maximumInstances: {}\n"
        "inboundQuota: {}\n"
        "outboundQuota: {}\n"
        "defaultTimeout: {}\n"
        "status: {}\n"
        "fileNameLength: {}\n"
        "fileName: {}\n"
        "{}"
        "\n",
        cmdId2str[(CMD_ID)namedPipeEvent.cmdId], sourceType2str[(SOURCE_TYPE)namedPipeEvent.sourceType],
        namedPipeEvent.threadId, namedPipeEvent.processId, namedPipeEvent.imagePath,
        "0x" + toHexStr(namedPipeEvent.fileObject), "0x" + toHexStr(namedPipeEvent.desiredAccess),
        "0x" + toHexStr(namedPipeEvent.fileAttributes), "0x" + toHexStr(namedPipeEvent.shareAccess),
        "0x" + toHexStr(namedPipeEvent.createDisposition), "0x" + toHexStr(namedPipeEvent.createOptions),
        "0x" + toHexStr(namedPipeEvent.namedPipeType), "0x" + toHexStr(namedPipeEvent.readMode),
        "0x" + toHexStr(namedPipeEvent.completionMode), "0x" + toHexStr(namedPipeEvent.maximumInstances),
        namedPipeEvent.inboundQuota, namedPipeEvent.outboundQuota, "0x" + toHexStr(namedPipeEvent.defaultTimeout),
        namedPipeEvent.status, namedPipeEvent.fileNameLength, namedPipeEvent.fileName, stackTrace);

    return TRUE;
}

BOOL NamedPipeEventDatabase::PrintCreateEvent(const NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::string stackTrace = "";
    if (!namedPipeEvent.stackTrace.empty())
    {
        stackTrace = "stackTrace: \n" + namedPipeEvent.stackTrace;
    }

    NPMLogger::LoggerInstance()->info(
        "\n"
        "cmdId: {}\n"
        "sourceType: {}\n"
        "threadId: {}\n"
        "processId: {}\n"
        "imagePath: {}\n"
        "fileObject: {}\n"
        "desiredAccess: {}\n"
        "shareAccess: {}\n"
        "createDisposition: {}\n"
        "createOptions: {}\n"
        "status: {}\n"
        "fileNameLength: {}\n"
        "fileName: {}\n"
        "{}"
        "\n",
        cmdId2str[(CMD_ID)namedPipeEvent.cmdId], sourceType2str[(SOURCE_TYPE)namedPipeEvent.sourceType],
        namedPipeEvent.threadId, namedPipeEvent.processId, namedPipeEvent.imagePath,
        "0x" + toHexStr(namedPipeEvent.fileObject), "0x" + toHexStr(namedPipeEvent.desiredAccess),
        "0x" + toHexStr(namedPipeEvent.shareAccess), "0x" + toHexStr(namedPipeEvent.createDisposition),
        "0x" + toHexStr(namedPipeEvent.createOptions), namedPipeEvent.status, namedPipeEvent.fileNameLength,
        namedPipeEvent.fileName, stackTrace);

    return TRUE;
}

BOOL NamedPipeEventDatabase::PrintReadEvent(const NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::string stackTrace = "";
    if (!namedPipeEvent.stackTrace.empty())
    {
        stackTrace = "stackTrace: \n" + namedPipeEvent.stackTrace;
    }

    NPMLogger::LoggerInstance()->info(
        "\n"
        "cmdId: {}\n"
        "sourceType: {}\n"
        "threadId: {}\n"
        "processId: {}\n"
        "imagePath: {}\n"
        "fileObject: {}\n"
        "status: {}\n"
        "fileNameLength: {}\n"
        "fileName: {}\n"
        "canImpersonate: {}\n"
        "bufferLength: {}\n"
        "buffer: \n{}"
        "{}"
        "\n",
        cmdId2str[(CMD_ID)namedPipeEvent.cmdId], sourceType2str[(SOURCE_TYPE)namedPipeEvent.sourceType],
        namedPipeEvent.threadId, namedPipeEvent.processId, namedPipeEvent.imagePath,
        "0x" + toHexStr(namedPipeEvent.fileObject), namedPipeEvent.status, namedPipeEvent.fileNameLength,
        namedPipeEvent.fileName, isTrue(namedPipeEvent.canImpersonate), namedPipeEvent.bufferLength,
        toXxdFormat(reinterpret_cast<const UCHAR*>(namedPipeEvent.buffer.data()), namedPipeEvent.buffer.size()), stackTrace);

    return TRUE;
}

BOOL NamedPipeEventDatabase::PrintWriteEvent(const NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::string stackTrace = "";
    if (!namedPipeEvent.stackTrace.empty())
    {
        stackTrace = "stackTrace: \n" + namedPipeEvent.stackTrace;
    }

    NPMLogger::LoggerInstance()->info(
        "\n"
        "cmdId: {}\n"
        "sourceType: {}\n"
        "threadId: {}\n"
        "processId: {}\n"
        "imagePath: {}\n"
        "fileObject: {}\n"
        "status: {}\n"
        "fileNameLength: {}\n"
        "fileName: {}\n"
        "canImpersonate: {}\n"
        "bufferLength: {}\n"
        "buffer: \n{}"
        "{}"
        "\n",
        cmdId2str[(CMD_ID)namedPipeEvent.cmdId], sourceType2str[(SOURCE_TYPE)namedPipeEvent.sourceType],
        namedPipeEvent.threadId, namedPipeEvent.processId, namedPipeEvent.imagePath,
        "0x" + toHexStr(namedPipeEvent.fileObject), namedPipeEvent.status, namedPipeEvent.fileNameLength,
        namedPipeEvent.fileName, isTrue(namedPipeEvent.canImpersonate), namedPipeEvent.bufferLength,
        toXxdFormat(reinterpret_cast<const UCHAR*>(namedPipeEvent.buffer.data()), namedPipeEvent.buffer.size()), stackTrace);

    return TRUE;
}

BOOL NamedPipeEventDatabase::PrintConnectNamedPipeEvent(const NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::string stackTrace = "";
    if (!namedPipeEvent.stackTrace.empty())
    {
        stackTrace = "stackTrace: \n" + namedPipeEvent.stackTrace;
    }

    NPMLogger::LoggerInstance()->info(
        "\n"
        "cmdId: {}\n"
        "sourceType: {}\n"
        "threadId: {}\n"
        "processId: {}\n"
        "imagePath: {}\n"
        "fileObject: {}\n"
        "status: {}\n"
        "fileNameLength: {}\n"
        "fileName: {}\n"
        "canImpersonate: {}\n"
        "{}"
        "\n",
        cmdId2str[(CMD_ID)namedPipeEvent.cmdId], sourceType2str[(SOURCE_TYPE)namedPipeEvent.sourceType],
        namedPipeEvent.threadId, namedPipeEvent.processId, namedPipeEvent.imagePath,
        "0x" + toHexStr(namedPipeEvent.fileObject), namedPipeEvent.status, namedPipeEvent.fileNameLength,
        namedPipeEvent.fileName, isTrue(namedPipeEvent.canImpersonate), stackTrace);

    return TRUE;
}

BOOL NamedPipeEventDatabase::PrintEvent(const NAMED_PIPE_EVENT& namedPipeEvent, BOOL toPrint)
{
    std::lock_guard<std::mutex> lock(monitorMutex);
    if (!toPrint)
    {
        return FALSE;
    }

    CMD_ID cmdId = (CMD_ID)namedPipeEvent.cmdId;

    switch (cmdId)
    {
        case CMD_CREATE_NAMED_PIPE:
            PrintCreateNamedPipeEvent(namedPipeEvent);
            break;

        case CMD_CREATE:
            PrintCreateEvent(namedPipeEvent);
            break;

        case CMD_READ:
            PrintReadEvent(namedPipeEvent);
            break;

        case CMD_WRITE:
            PrintWriteEvent(namedPipeEvent);
            break;

        case CMD_CONNECT_NAMED_PIPE:
            PrintConnectNamedPipeEvent(namedPipeEvent);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL NamedPipeEventDatabase::StoreEvent(NAMED_PIPE_EVENT& namedPipeEvent)
{
    // Filter named pipe events made by ourselves and anonymous pipes.
    if (namedPipeEvent.fileName.find(toUTF8(NAMED_PIPE_MASTER)) != std::string::npos ||
        namedPipeEvent.fileName == toUTF8(RING0_ANONYMOUS_PIPE) || namedPipeEvent.fileName == toUTF8(RING3_ANONYMOUS_PIPE))
    {
        return FALSE;
    }

    // PrintEvent(namedPipeEvent, monitorMode);
    PushEvent(namedPipeEvent);

    return TRUE;
}

VOID NamedPipeEventDatabase::PushEvent(NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(namedPipeEvent);
}

NAMED_PIPE_EVENT NamedPipeEventDatabase::PopEvent()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    NAMED_PIPE_EVENT namedPipeEvent = eventQueue.front();
    eventQueue.pop();
    return namedPipeEvent;
}

DWORD NamedPipeEventDatabase::GetQueueSize()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return eventQueue.size();
}

BOOL NamedPipeEventDatabase::InsertDatabase(NAMED_PIPE_EVENT& namedPipeEvent)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    try
    {
        namedPipeEvent.id = storage->insert(namedPipeEvent);
        dbSize++;
    }
    catch (...)
    {
        NPMLogger::LoggerInstance()->error("[{}] insert error", __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

BOOL NamedPipeEventDatabase::ClearDatabase()
{
    std::lock_guard<std::mutex> lock(dbMutex);

    try
    {
        storage->remove_all<NAMED_PIPE_EVENT>();
        dbSize = 0;
    }
    catch (...)
    {
        NPMLogger::LoggerInstance()->error("[{}] remove_all error", __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

BOOL NamedPipeEventDatabase::RotateDatabase()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    try
    {
        // Remove 10% old records in database.
        storage->remove_all<NAMED_PIPE_EVENT>(
            where(in(&NAMED_PIPE_EVENT::id,
                     select(&NAMED_PIPE_EVENT::id, order_by(&NAMED_PIPE_EVENT::id).asc(), limit(maxDbSize / 10)))));
        dbSize = storage->count<NAMED_PIPE_EVENT>();
    }
    catch (...)
    {
        NPMLogger::LoggerInstance()->error("[{}] remove_all error", __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

DWORD NamedPipeEventDatabase::GetDatabaseSize()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    return dbSize;
}

std::vector<NAMED_PIPE_EVENT> NamedPipeEventDatabase::SelectDatabase(ULONG id)
{
    std::lock_guard<std::mutex> lock(dbMutex);

    std::vector<NAMED_PIPE_EVENT> results;
    try
    {
        auto conditionId = id == 0 or c(&NAMED_PIPE_EVENT::id) >= id;
        auto conditionCmdId = namedPipeEventFilter.cmdId == 0 or c(&NAMED_PIPE_EVENT::cmdId) == namedPipeEventFilter.cmdId;
        auto conditionSourceType =
            namedPipeEventFilter.sourceType == 0 or c(&NAMED_PIPE_EVENT::sourceType) == namedPipeEventFilter.sourceType;
        auto conditionprocessId =
            namedPipeEventFilter.processId == 0 or c(&NAMED_PIPE_EVENT::processId) == namedPipeEventFilter.processId;
        auto conditionImagePath = like(&NAMED_PIPE_EVENT::imagePath, "%" + namedPipeEventFilter.imagePath + "%");
        auto conditionFileName = like(&NAMED_PIPE_EVENT::fileName, "%" + namedPipeEventFilter.fileName + "%");
        auto conditionCanImpersonate = namedPipeEventFilter.canImpersonate == FALSE or
                                       c(&NAMED_PIPE_EVENT::canImpersonate) == namedPipeEventFilter.canImpersonate;
        results = storage->get_all<NAMED_PIPE_EVENT>(
            where(conditionId and conditionCmdId and conditionSourceType and conditionprocessId and conditionImagePath and
                  conditionFileName and conditionCanImpersonate),
            order_by(&NAMED_PIPE_EVENT::id).asc());
    }
    catch (...)
    {
        NPMLogger::LoggerInstance()->error("[{}] remove_all error", __FUNCTION__);
    }

    return results;
}

std::vector<std::vector<std::string>> NamedPipeEventDatabase::GetUniqueImagePathAndFileNamePairs()
{
    std::lock_guard<std::mutex> lock(dbMutex);

    std::vector<std::vector<std::string>> results;
    try
    {
        auto uniquePairs = storage->select(distinct(columns(&NAMED_PIPE_EVENT::imagePath, &NAMED_PIPE_EVENT::fileName)));
        for (auto& uniquePair : uniquePairs)
        {
            std::vector<std::string> pair({std::get<0>(uniquePair), std::get<1>(uniquePair)});
            results.push_back(pair);
        }
    }
    catch (...)
    {
        NPMLogger::LoggerInstance()->error("[{}] remove_all error", __FUNCTION__);
    }

    return results;
}

VOID NamedPipeEventDatabase::SetFilter(const NAMED_PIPE_EVENT_FILTER& _namedPipeEventFilter)
{
    namedPipeEventFilter = _namedPipeEventFilter;
}

VOID NamedPipeEventDatabase::ResetFilter()
{
    namedPipeEventFilter.cmdId = 0;
    namedPipeEventFilter.sourceType = 0;
    namedPipeEventFilter.processId = 0;
    namedPipeEventFilter.imagePath = "";
    namedPipeEventFilter.fileName = "";
    namedPipeEventFilter.canImpersonate = FALSE;
}

NAMED_PIPE_EVENT_FILTER NamedPipeEventDatabase::GetFilter()
{
    return namedPipeEventFilter;
}


ULONG NamedPipeEventDatabase::GetHighestPrintedId()
{
    return highestPrintedId;
}

VOID NamedPipeEventDatabase::SetHighestPrintedId(ULONG _highestPrintedId)
{
    highestPrintedId = _highestPrintedId;
}
