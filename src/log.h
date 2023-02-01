#pragma once

#include <string>
#include <windows.h>

#define LDEBUG   0
#define LINFO    1
#define LERROR   2
#define LNOLOG   9999

class Logger
{
public:
    enum LogMode
    {
        DBGVIEW   = 0,      ///< OutputDebugStr´òÓ¡ÈÕÖ¾
        //LOGFILE   = 1,
    };

    enum LogLevel
    {
        LOG_LDEBUG   = LDEBUG,
        LOG_LINFO    = LINFO,
        LOG_LERROR   = LERROR,
        LOG_LNOLOG   = LNOLOG,
    };

public:
    Logger();
    ~Logger();

    static bool init_log(LogMode mode, LogLevel level);
    static void set_log_level(LogLevel level);
    // void LogMessage(std::string message);
    static void log_message(LogLevel level, 
                           const char *pFilename, 
                           int lineNo, 
                           const char *pFunctionName, 
                           const char *pFormat, ...);

private:
    static LogLevel cur_level_;
    static LogMode log_mode_;
    static HANDLE hLogFile_;
};

#define LOG(loglevel, formatstr, ...) Logger::log_message(Logger::LOG_##loglevel, formatstr, ##__VA_ARGS__);

#define LOG_DEBUG(formatstr, ...) do\
    {\
        Logger::log_message(Logger::LOG_LDEBUG, __FILE__, __LINE__, __FUNCTION__, formatstr, ##__VA_ARGS__);\
    } while (false)

#define LOG_INFO(formatstr, ...) do\
    {\
        Logger::log_message(Logger::LOG_LINFO, __FILE__, __LINE__, __FUNCTION__, formatstr, ##__VA_ARGS__);\
    } while (false)

#define LOG_ERROR(formatstr, ...) do\
    {\
        Logger::log_message(Logger::LOG_LERROR, __FILE__, __LINE__, __FUNCTION__, formatstr, ##__VA_ARGS__);\
    } while (false)

