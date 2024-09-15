#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "os/os.h"
#include "log/log.h"

#include <stdio.h>
#include <time.h>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT ""
#endif
#define STRLOG_NO_COLOR
void log__writeTimestamp(Arena* arena, log_Severity severity, S8 fileName, u64 line) {
    log_Severity sev = clampVal(severity, log_severity_trace, log_severity_fatal);
    S8 log__severityLevelStrings[] = {
        str_lit("TRACE"), str_lit("DEBUG"), str_lit("INFO "), str_lit("WARN "), str_lit("ERROR"), str_lit("FATAL")
    };

    S8 log__severityLevelColors[] = {
        str_lit("\x1b[94m"), str_lit("\x1b[36m"), str_lit("\x1b[32m"), str_lit("\x1b[33m"), str_lit("\x1b[31m"), str_lit("\x1b[35m")
    };
    S8 severityStr = log__severityLevelStrings[sev];
    S8 severityColor = log__severityLevelColors[sev];
    time_t t = time(NULL);
    struct tm *time = localtime(&t);
    char timeBuffer[64];
    i32 size = strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", time);
    timeBuffer[size] = '\0';
    S8 timeStr;
    timeStr.size = size;
    timeStr.content = (u8*) &timeBuffer[0];
    S8 log__projectRootPath = str_lit(PROJECT_ROOT "/");
    S8 relativeFilePath = str_subStr(fileName, log__projectRootPath.size, fileName.size - log__projectRootPath.size);
    #ifndef STRLOG_NO_COLOR
    S8 COL0 = str_lit("\x1b[0m\x1b[90m");
    S8 COL1 = str_lit("\x1b[0m");
    str_join(arena, timeStr, s8(" "), severityColor, severityStr, s8(" "), COL0, relativeFilePath, s8(":"), line, s8(":"), COL1, s8(" "));
    #else
    str_join(arena, timeStr, s8(" "), severityStr, s8(" "), relativeFilePath, s8(":"), line, s8(": "));
    #endif
}

void log__msg(Arena* mem, log_Severity severity, S8 fileName, u64 line, u32 argCount, ...) {
    va_list valist;
    va_start(valist, argCount);
    mem_scoped(scratch, mem) {
        S8 logStr;
        str_record(logStr, scratch.arena) {
            log__writeTimestamp(scratch.arena, severity, fileName, line);
            str_joinVargs(scratch.arena, argCount, valist);
            str_join(scratch.arena, s8("\r\n"), STR_TERMINATOR);
        }
        os_log(logStr);
    }
}

void log__msgFmt(Arena* mem, log_Severity severity, S8 fileName, u64 line, S8 template, u32 argCount, ...) {
    va_list valist;
    va_start(valist, argCount);
    mem_scoped(scratch, mem) {
        S8 logStr;
        str_record(logStr, scratch.arena) {
            log__writeTimestamp(scratch.arena, severity, fileName, line);
            str_fmtVargs(scratch.arena, template, argCount, valist);
            str_join(scratch.arena, s8("\r\n"), STR_TERMINATOR);
        }
        os_log(logStr);
    }
}