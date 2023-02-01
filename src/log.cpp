#include "log.h"

#include <time.h>
#include <conio.h>
#include <stdio.h>
#include <locale>

Logger::LogLevel Logger::cur_level_ = LOG_LERROR;
Logger::LogMode Logger::log_mode_ = DBGVIEW;
HANDLE Logger::hLogFile_ = NULL;

Logger::Logger()
    // : cur_level_(LOG_LFATAL)
{
}

Logger::~Logger()
{
}

bool Logger::init_log(LogMode mode, LogLevel level)
{
	UNREFERENCED_PARAMETER(level);
#ifdef PUBLOG
	set_log_level(LOG_LINFO);
#else
    set_log_level(level);
#endif
    log_mode_ = mode;

    if (mode & DBGVIEW)
    {
        // InitLogDbgView();
    }

    return true;
}

void Logger::set_log_level(LogLevel level)
{
    cur_level_ = level;
}

bool bLogFileName = false;
bool bLogFunctionName = true;
const int kMaxLogLineLen = 2048;
const int kMaxLogStreamLen = 20480;
void Logger::log_message(LogLevel level, const char *pFilename, int lineNo, const char *pFunctionName, const char *pFormat, ...)
{
    UNREFERENCED_PARAMETER(lineNo);

    if (level < cur_level_)
    {
        //No Need To LoggerOut
        return;
    }

    if (!pFormat)
    {
        return;
    }

    //size_t logstringlen = cprintf(pFormat, ...);
    char *pszMsg = (char*)malloc(kMaxLogLineLen);
    if (NULL == pszMsg)
    {
        return ;
    }
    memset(pszMsg, 0, kMaxLogLineLen);

    //if (strlen(szPrefix))
    //{
    //    strcat_s(pszMsg, DEBUGOUT_STRING_BUFFER_LEN, szPrefix);
    //}

    switch (level) {
    case LOG_LDEBUG:
        strcat_s(pszMsg, kMaxLogLineLen, "[DEBUG]");
        break;
    case LOG_LERROR:
        strcat_s(pszMsg, kMaxLogLineLen, "[ERROR]");
        break;
    case LOG_LINFO:
        strcat_s(pszMsg, kMaxLogLineLen, "[INFO]");
        break;
    }

    if (bLogFileName)
    {
        strcat_s(pszMsg, kMaxLogLineLen, "[");
        strcat_s(pszMsg, kMaxLogLineLen, pFilename);
        strcat_s(pszMsg, kMaxLogLineLen, "]");
    }
    if (bLogFunctionName)
    {
        strcat_s(pszMsg, kMaxLogLineLen, "[");
        strcat_s(pszMsg, kMaxLogLineLen, (pFunctionName));
        strcat_s(pszMsg, kMaxLogLineLen, "]");
    }

    size_t ulMsgLen = strlen(pszMsg);
    va_list ap;
    va_start(ap, pFormat);

    _vsnprintf_s(pszMsg + ulMsgLen, 
                 kMaxLogLineLen - 1 - ulMsgLen, 
                 kMaxLogLineLen - 1 - ulMsgLen, 
                 pFormat, ap);

    va_end(ap);

    ulMsgLen = strlen(pszMsg);

    if (pszMsg[ulMsgLen - 1] != '\n')
    {
        if (ulMsgLen < kMaxLogLineLen - 2)
        {
            pszMsg[ulMsgLen]    = '\r';
            pszMsg[ulMsgLen + 1] = '\n';
            pszMsg[ulMsgLen + 2] = '\0';
        }
    }

    OutputDebugStringA(pszMsg);
    //WriteToLogFile(pszMsg);

    free(pszMsg);
    pszMsg = NULL;

}
