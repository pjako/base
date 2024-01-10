#include "base/base.h"
#include "base/base_types.h"
#include "base/base_str.h"
#include "os/os.h"
#include "log/log.h"

#include <stdio.h>
#include <time.h>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT ""
#endif

void log__writeTimestamp(Arena* arena, log_Severity severity, Str8 fileName, u64 line) {
    log_Severity sev = maxVal(minVal(severity, log_severity_fatal), log_severity_trace);
    Str8 log__severityLevelStrings[] = {
        str_lit("TRACE"), str_lit("DEBUG"), str_lit("INFO "), str_lit("WARN "), str_lit("ERROR"), str_lit("FATAL")
    };

    Str8 log__severityLevelColors[] = {
        str_lit("\x1b[94m"), str_lit("\x1b[36m"), str_lit("\x1b[32m"), str_lit("\x1b[33m"), str_lit("\x1b[31m"), str_lit("\x1b[35m")
    };
    Str8 severityStr = log__severityLevelStrings[sev];
    Str8 severityColor = log__severityLevelColors[sev];
    time_t t = time(NULL);
    struct tm *time = localtime(&t);
    char timeBuffer[64];
    i32 size = strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", time);
    timeBuffer[size] = '\0';
    Str8 timeStr;
    timeStr.size = size;
    timeStr.content = (u8*) &timeBuffer[0];
    Str8 log__projectRootPath = str_lit(PROJECT_ROOT "/");
    Str8 relativeFilePath = str_subStr(fileName, log__projectRootPath.size, fileName.size - log__projectRootPath.size);
    #ifndef STRLOG_NO_COLOR
    Str8 COL0 = str_lit("\x1b[0m\x1b[90m");
    Str8 COL1 = str_lit("\x1b[0m");
    str_join(arena, timeStr, " ", severityColor, severityStr, " ", COL0, relativeFilePath, ":", line, ":", COL1, " ");
    #else
    str_join(arena, timeStr, " ", severityStr, " ", relativeFilePath, ":", line, ": ");
    #endif
}

void log__msg(Arena* mem, log_Severity severity, Str8 fileName, u64 line, u32 argCount, ...) {
    va_list valist;
    va_start(valist, argCount);
    mem_scoped(scratch, mem) {
        Str8 logStr;
        str_record(logStr, scratch.arena) {
            log__writeTimestamp(scratch.arena, severity, fileName, line);
            str_joinVargs(scratch.arena, argCount, valist);
            str_join(scratch.arena, "\r\n", STR_TERMINATOR);
        }
        os_log(logStr);
    }
}

void log__msgFmt(Arena* mem, log_Severity severity, Str8 fileName, u64 line, Str8 template, u32 argCount, ...) {
    va_list valist;
    va_start(valist, argCount);
    mem_scoped(scratch, mem) {
        Str8 logStr;
        str_record(logStr, scratch.arena) {
            log__writeTimestamp(scratch.arena, severity, fileName, line);
            str_fmtVargs(scratch.arena, template, argCount, valist);
            str_join(scratch.arena, "\r\n", STR_TERMINATOR);
        }
        os_log(logStr);
    }
}