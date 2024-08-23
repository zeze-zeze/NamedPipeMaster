# NamedPipeMaster
<p align="center">
  <img src="assets/logo.jpg" width="200px">
</p>

## Description
NamedPipeMaster is a versatile tool for analyzing and monitoring in named pipes. It includes Ring3NamedPipeConsumer for direct server interaction, Ring3NamedPipeMonitor for DLL-based API hooking and data collection, and Ring0NamedPipeFilter for comprehensive system-wide monitoring. The tool supports proactive and passive interactions, collects detailed communication data, and features a filter for specific event searches.

## Features
- Named Pipe Interaction:
    - Proactive Interaction: Actively interact with a named pipe server.
    - Passive Connection: Be passively connected by a named pipe client.
    - Proxy Interaction: Inject a DLL into a process to serve as a proxy for interacting with a named pipe server.
- Information Collection via DLL Injection (Ring3 Hook):
    - Monitors and collects information on named pipe communication by hooking relevant APIs.
    - Dumps the call stack in detoured functions and checks the process's impersonation capability.
    - Specific API hooks include:
        - NtCreateNamedPipeFile: Named pipe creation.
        - NtCreateFile: Named pipe connection.
        - NtFsControlFile: Named pipe connection completion.
        - NtReadFile: Reading data from a named pipe.
        - NtWriteFile: Writing data to a named pipe.
- System-Wide Monitoring with Minifilter Driver:
    - Captures system-wide named pipe activities by monitoring key IRPs (I/O Request Packets):
        - IRP_MJ_CREATE_NAMED_PIPE: Named pipe creation.
        - IRP_MJ_CREATE: Named pipe connection.
        - IRP_MJ_FILE_SYSTEM_CONTROL: Named pipe connection completion.
        - IRP_MJ_READ: Reading data from a named pipe.
        - IRP_MJ_WRITE: Writing data to a named pipe.


## Usage
Put Ring3NamedPipeConsumer.exe, Ring3NamedPipeMonitor.dll, and Ring0NamedPipeFilter.sys in the same directory and run Ring3NamedPipeConsumer.exe.
Please ensure that code integrity is disabled to use the features of the minifilter driver.

```
> Ring3NamedPipeConsumer.exe
[1] dump database
[2] start monitor mode
[3] clear database
[4] get database info
[5] filter
[6] inject dll
[7] NamedPipePoker
[8] NamedPipeProxyPoker
[9] NamedPipePoked
[10] help
[11] exit and clean up

NPM-CLI> 10
[1] dump database: print all monitored events in the log
[2] start monitor mode: keep monitoring named pipe activities until enter is pressed
[3] clear database: clear the log
[4] get database info: get some statistics
[5] filter: get the specified named pipe events
[6] inject dll: inject Ring3NamedPipeMonitor.dll into a process
[7] NamedPipePoker: directly interact with a named pipe server
[8] NamedPipeProxyPoker: inject Ring3NamedPipeMonitor.dll into a process as a proxy to interact with the target named pipe server
[9] NamedPipePoked: act as a named pipe server to be connected by other clients
[10] help: print this detail usage
[11] exit and clean up: terminate this process and unload the driver
```


## Build
- Visual Studio 2017
- C++17
- vcpkg v1.2.2
    - nlohmann-json
    - minhook
    - spdlog
    - sqlite-orm
    - cli11


## Reference
- [kobykahane/NpEtw](https://github.com/kobykahane/NpEtw)
- [Offensive Windows IPC Internals 1: Named Pipes](https://csandker.io/2021/01/10/Offensive-Windows-IPC-1-NamedPipes.html)
- [InterProcessCommunication-Samples](https://github.com/csandker/InterProcessCommunication-Samples)