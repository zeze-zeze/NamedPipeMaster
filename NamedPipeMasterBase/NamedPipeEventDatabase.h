#pragma once
#include <sqlite_orm/sqlite_orm.h>
#include <Windows.h>
#include <mutex>
#include <queue>
#include "NamedPipeProtocol.h"

struct NAMED_PIPE_EVENT
{
    ULONG id;
    std::string currentTime;
    ULONG cmdId;         // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    ULONG sourceType;    // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    ULONG threadId;      // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    ULONG processId;     // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    std::string imagePath;
    ULONG64 fileObject;         // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    ULONG desiredAccess;        // DATA_CREATE_NAMED_PIPE, DATA_CREATE
    ULONG fileAttributes;       // DATA_CREATE
    ULONG shareAccess;          // DATA_CREATE_NAMED_PIPE, DATA_CREATE
    ULONG createDisposition;    // DATA_CREATE_NAMED_PIPE, DATA_CREATE
    ULONG createOptions;        // DATA_CREATE_NAMED_PIPE, DATA_CREATE
    ULONG namedPipeType;        // DATA_CREATE_NAMED_PIPE
    ULONG readMode;             // DATA_CREATE_NAMED_PIPE
    ULONG completionMode;       // DATA_CREATE_NAMED_PIPE
    ULONG maximumInstances;     // DATA_CREATE_NAMED_PIPE
    ULONG inboundQuota;         // DATA_CREATE_NAMED_PIPE
    ULONG outboundQuota;        // DATA_CREATE_NAMED_PIPE
    ULONG64 defaultTimeout;     // DATA_CREATE_NAMED_PIPE
    ULONG status;
    ULONG fileNameLength;        // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    std::string fileName;        // DATA_CREATE_NAMED_PIPE, DATA_CREATE, DATA_READ, DATA_WRITE
    ULONG bufferLength;          // DATA_READ, DATA_WRITE
    std::vector<CHAR> buffer;    // DATA_READ, DATA_WRITE
    bool canImpersonate;         // DATA_CONNECT_NAMED_PIPE
    std::string stackTrace;      // SOURCE_INJECTED_DLL
};

struct NAMED_PIPE_EVENT_FILTER
{
    ULONG cmdId;
    ULONG sourceType;
    ULONG processId;
    std::string imagePath;
    std::string fileName;
    bool canImpersonate;
};


inline auto InitStorage(std::string path = ":memory:")
{
    using namespace sqlite_orm;
    return make_storage(
        path,
        make_table(
            "NamedPipeEvent", make_column("id", &NAMED_PIPE_EVENT::id, autoincrement(), primary_key()),
            make_column("currentTime", &NAMED_PIPE_EVENT::currentTime), make_column("cmdId", &NAMED_PIPE_EVENT::cmdId),
            make_column("sourceType", &NAMED_PIPE_EVENT::sourceType), make_column("threadId", &NAMED_PIPE_EVENT::threadId),
            make_column("processId", &NAMED_PIPE_EVENT::processId), make_column("imagePath", &NAMED_PIPE_EVENT::imagePath),
            make_column("fileObject", &NAMED_PIPE_EVENT::fileObject),
            make_column("desiredAccess", &NAMED_PIPE_EVENT::desiredAccess),
            make_column("fileAttributes", &NAMED_PIPE_EVENT::fileAttributes),
            make_column("shareAccess", &NAMED_PIPE_EVENT::shareAccess),
            make_column("createDisposition", &NAMED_PIPE_EVENT::createDisposition),
            make_column("createOptions", &NAMED_PIPE_EVENT::createOptions),
            make_column("namedPipeType", &NAMED_PIPE_EVENT::namedPipeType),
            make_column("readMode", &NAMED_PIPE_EVENT::readMode),
            make_column("completionMode", &NAMED_PIPE_EVENT::completionMode),
            make_column("maximumInstances", &NAMED_PIPE_EVENT::maximumInstances),
            make_column("inboundQuota", &NAMED_PIPE_EVENT::inboundQuota),
            make_column("outboundQuota", &NAMED_PIPE_EVENT::outboundQuota),
            make_column("defaultTimeout", &NAMED_PIPE_EVENT::defaultTimeout),
            make_column("status", &NAMED_PIPE_EVENT::status),
            make_column("fileNameLength", &NAMED_PIPE_EVENT::fileNameLength),
            make_column("fileName", &NAMED_PIPE_EVENT::fileName),
            make_column("bufferLength", &NAMED_PIPE_EVENT::bufferLength), make_column("buffer", &NAMED_PIPE_EVENT::buffer),
            make_column("canImpersonate", &NAMED_PIPE_EVENT::canImpersonate),
            make_column("stackTrace", &NAMED_PIPE_EVENT::stackTrace)));
}
using NamedPipeEventStorage = decltype(InitStorage());

class NamedPipeEventDatabase
{
  private:
    std::mutex dbMutex, queueMutex, monitorMutex;
    DWORD dbSize = 0, maxDbSize = 65535;
    std::queue<NAMED_PIPE_EVENT> eventQueue;
    std::unique_ptr<NamedPipeEventStorage> storage;
    BOOL monitorMode;
    ULONG highestPrintedId;
    NAMED_PIPE_EVENT_FILTER namedPipeEventFilter;

  private:
    BOOL PrintCreateNamedPipeEvent(const NAMED_PIPE_EVENT& namedPipeEvent);
    BOOL PrintCreateEvent(const NAMED_PIPE_EVENT& namedPipeEvent);
    BOOL PrintReadEvent(const NAMED_PIPE_EVENT& namedPipeEvent);
    BOOL PrintWriteEvent(const NAMED_PIPE_EVENT& namedPipeEvent);
    BOOL PrintConnectNamedPipeEvent(const NAMED_PIPE_EVENT& namedPipeEvent);

  public:
    NamedPipeEventDatabase();
    static NamedPipeEventDatabase& Instance()
    {
        static NamedPipeEventDatabase namedPipeEventDatabase;
        return namedPipeEventDatabase;
    }

    BOOL InitializeDatabase();
    VOID SetMaxDbSize(DWORD _maxDbSize);
    DWORD GetMaxDbSize();
    VOID SetMonitorMode(BOOL _monitorMode);
    BOOL GetMonitorMode();
    BOOL PrintEvent(const NAMED_PIPE_EVENT& namedPipeEvent, BOOL toPrint);
    BOOL StoreEvent(NAMED_PIPE_EVENT& namedPipeEvent);
    VOID PushEvent(NAMED_PIPE_EVENT& namedPipeEvent);
    NAMED_PIPE_EVENT PopEvent();
    BOOL InsertDatabase(NAMED_PIPE_EVENT& namedPipeEvent);
    BOOL ClearDatabase();
    BOOL RotateDatabase();
    DWORD GetQueueSize();
    DWORD GetDatabaseSize();
    std::vector<NAMED_PIPE_EVENT> SelectDatabase(ULONG id = 0);
    std::vector<std::vector<std::string>> GetUniqueImagePathAndFileNamePairs();
    VOID SetFilter(const NAMED_PIPE_EVENT_FILTER& _namedPipeEventFilter);
    VOID ResetFilter();
    NAMED_PIPE_EVENT_FILTER GetFilter();
    ULONG GetHighestPrintedId();
    VOID SetHighestPrintedId(ULONG _highestPrintedId);
};
