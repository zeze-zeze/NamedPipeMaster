#pragma once
#include <Windows.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

class NPMLogger
{
  public:
    std::shared_ptr<spdlog::logger> logger;
    spdlog::level::level_enum logLevel;

    NPMLogger()
    {
        logLevel = spdlog::level::info;

        try
        {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(logLevel);

            // Max size for the log file is 5 MB.
            DWORD max_size = 1024 * 1024 * 5;
            DWORD max_files = 0;
            auto rotateFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("NPM.log", max_size, max_files);
            rotateFileSink->set_level(logLevel);

            logger =
                std::make_shared<spdlog::logger>(spdlog::logger("NamedPipeMasterLogger", {consoleSink, rotateFileSink}));
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "[" << __FUNCTION__ << "]"
                      << "log init error: " << ex.what() << std::endl;
        }
    }

    static NPMLogger& Instance()
    {
        static NPMLogger npmLogger;
        return npmLogger;
    }

    static std::shared_ptr<spdlog::logger>& LoggerInstance()
    {
        return Instance().logger;
    }

    VOID SetLogLevel(spdlog::level::level_enum _logLevel)
    {
        logLevel = _logLevel;
        for (auto& sink : logger->sinks())
        {
            sink->set_level(_logLevel);
        }
    }
};
