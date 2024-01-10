
#ifndef LOG_H
#define LOG_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum log_Severity {
    log_severity_trace   = 0,
    log_severity_debug   = 1,
    log_severity_info    = 2,
    log_severity_warning = 3,
    log_severity_error   = 4,
    log_severity_fatal   = 5,
} log_Severity;

#ifndef LOG_SEVERITY_MIN
#define LOG_SEVERITY_MIN log_severity_trace
#endif // LOG_SEVERITY_MIN

#if LOG_SEVERITY_MIN <= log_severity_trace
#define log_traceFmt(ARENA, TEMPLATE, ...) log_msgFmt(ARENA, log_severity_trace, TEMPLATE, __VA_ARGS__)
#define log_trace(ARENA, ...) log_msg(ARENA, log_severity_trace, __VA_ARGS__)
#else
#define log_traceFmt(...) 
#define log_trace(...) 
#endif
#if LOG_SEVERITY_MIN <= log_severity_debug
#define log_debugFmt(ARENA, TEMPLATE, ...) log_msgFmt(ARENA, log_severity_debug, TEMPLATE, __VA_ARGS__)
#define log_debug(ARENA, ...) log_msg(ARENA, log_severity_debug, __VA_ARGS__)
#else
#define log_debugFmt(...) 
#define log_debug(...) 
#endif

#if LOG_SEVERITY_MIN <= log_severity_info
#define log_infoFmt(ARENA, TEMPLATE, ...) log_msgFmt(ARENA, log_severity_info, TEMPLATE, __VA_ARGS__)
#define log_info(ARENA, ...) log_msg(ARENA, log_severity_info, __VA_ARGS__)
#else
#define log_infoFmt(...) 
#define log_info(...) 
#endif

#if LOG_SEVERITY_MIN <= log_severity_warning
#define log_warnFmt(ARENA, EMPLATE, ...) log_msgFmt(ARENA, log_severity_warning, TEMPLATE, __VA_ARGS__)
#define log_warn(ARENA, ...) log_msg(ARENA, log_severity_warning, __VA_ARGS__)
#else
#define log_warnFmt(...) 
#define log_warn(...) 
#endif

#if LOG_SEVERITY_MIN <= log_severity_error
#define log_errorFmt(ARENA, TEMPLATE, ...) log_msgFmt(ARENA, log_severity_error, TEMPLATE, __VA_ARGS__)
#define log_error(ARENA, ...) log_msg(ARENA, log_severity_error, __VA_ARGS__)
#else
#define log_errorFmt(...) 
#define log_error(...) 
#endif

#if LOG_SEVERITY_MIN <= log_severity_fatal
#define log_fatalFmt(ARENA, TEMPLATE, ...) log_msgFmt(ARENA, log_severity_fatal, TEMPLATE, __VA_ARGS__)
#define log_fatal(ARENA, ...) log_msg(ARENA, log_severity_fatal, __VA_ARGS__)
#else
#define log_fatalFmt(...) 
#define log_fatal(...) 
#endif

#define log_msg(ARENA, SEVERITY, ...) STR_ARG_OVER_UNDER_FLOW_CHECKER(log__msg, STR_AT_LEAST_TWO_ARGS, __VA_ARGS__)(ARENA, SEVERITY, str_lit(__FILE__), __LINE__, STR_ARR_MACRO_CHOOSER(__VA_ARGS__)(str_Value, str__convertToValue, __VA_ARGS__))
#define log_msgFmt(ARENA, severity, TEMPLATE, ...) STR_ARG_OVER_UNDER_FLOW_CHECKER(log__msgFmt, STR_AT_LEAST_TWO_ARGS, __VA_ARGS__)(ARENA, severity, str_lit(__FILE__), __LINE__, str__convertToKey(TEMPLATE), STR_ARR_MACRO_CHOOSER(__VA_ARGS__)(str_KeyValue, str__convertToKeyValue, __VA_ARGS__))

API void log__msg(Arena* tmpArena, log_Severity severity, Str8 fileName, u64 line, u32 argCount, ...);
API void log__msgFmt(Arena* tmpArena, log_Severity severity, Str8 fileName, u64 line, Str8 strTemplate, u32 argCount, ...);

// API Str8 log_pull(void);
typedef void (log_callbackFn) (log_Severity severity, Str8 fileName, u64 line, Str8 str, void* user);
API void log_setCallback(log_callbackFn* callback, void* user);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // LOG_H