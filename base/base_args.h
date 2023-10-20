#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum arg_errors {
    arg_error_duplicate    = '&',
    arg_error_missingArg   = '=',
    arg_error_missingValue = '+',
    arg_error_invalidValue = '!',
} arg_errors;

static const i32 arg_end = -1;

typedef enum arg_flags {
    arg_flag_none               = 0,
    arg_flag_required           = 1 << 0,
    arg_flag_requiredValue      = 1 << 1,
    arg_flag_duplicatesAllowed  = 1 << 2,
} arg_flags;

typedef enum arg_optType {
    arg_optType_none,
    arg_optType_string,
    arg_optType_f32,
    arg_optType_i32,
    arg_optType_flags,
} arg_optType;

typedef struct arg_Opt {
    Str8 name;
    i32 shortName;
    arg_optType type;
    u32 flags;
    Str8 defaultValue;
    Str8 desc;
    Str8 helpText;
} arg_Opt;

typedef struct arg_Ctx {
    arg_Opt* opts;
    u32 optCount;
    char** inputOpts;
    u32 inputOptsCount;
    u64 redArgsBitField;
    u32 nextIdx;
    arg_Opt* currentOpt;
    Str8 foundKey;
    Str8 foundValue;
    i32 foundi32Value;
    f32 foundF32Value;
    u32 foundFlagsValue;
} arg_Ctx;

API arg_Ctx arg_makeCtx(arg_Opt* opts, u32 optsCount, char** inputOpts, u32 inputOptsCount);

API Str8 arg_createHelpText(Arena* arena, arg_Ctx* ctx);

API i32 arg_nextOpt(arg_Ctx* ctx);

#ifdef __cplusplus
}
#endif
#endif /* ARGS_H */