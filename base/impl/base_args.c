#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"

#include "base/base_args.h"

arg_Ctx arg_makeCtx(arg_Opt* opts, u32 optsCount, char** inputOpts, u32 inputOptsCount) {
    arg_Ctx ctx = {opts, optsCount, inputOpts, inputOptsCount};
    return ctx;
}

Str8 arg_createHelpText(Arena* arena, arg_Ctx* ctx) {
    for (u64 idx = 0; idx < ctx->optCount; idx++) {
        arg_Opt* opt = ctx->opts + idx;
        str_join(arena, str8("argument:"), opt->name, str8(" or "), opt->shortName, str8("\n"));
    }
    return str8("");
}

i32 arg_nextOpt(arg_Ctx* ctx) {
    while (ctx->nextIdx < ctx->inputOptsCount) {
        char* charOpt = ctx->inputOpts[ctx->nextIdx];
        Str8 key = str_fromNullTerminatedCharPtr(charOpt);
        //i64 firstSpace = str_findChar(opt, ' ');
        //Str8 key;
        bx longName = false;
        if (str_hasPrefix(key, str8("--"))) {
            longName = true;
            key = str_subStr(key, 2, key.size);
            if (key.size == 0) {
                ctx->nextIdx++;
                continue;
            }
        } else if (str_hasPrefix(key, str8("-")) && key.size > 1) {
            key = str_subStr(key, 1, key.size);
        } else {
            // skip arguments that don't start with "-" or "--"
            ctx->nextIdx++;
            continue;
        }
        Str8 value = STR_EMPTY;
        if ((ctx->nextIdx + 1) < ctx->inputOptsCount) {
            char* charOpt = ctx->inputOpts[ctx->nextIdx + 1];
            if (charOpt[0] != '-') {
                value = str_fromNullTerminatedCharPtr(charOpt);
                ctx->nextIdx++;
            }
        }
        
        for (u64 idx = 0; idx < ctx->optCount; idx++) {
            arg_Opt* opt = ctx->opts + idx;
            if (longName ? str_isEqual(key, opt->name) : (key.size == 1 && key.content[0] == opt->shortName)) {
                ctx->foundKey   = opt->name;
                ctx->foundValue = value.size > 0 ? value : opt->defaultValue;
                ctx->foundi32Value  = 0;
                ctx->foundF32Value  = 0;
                ctx->foundFlagsValue = 0;
                ctx->currentOpt = opt;
                ctx->nextIdx++;

                // check for duplicate
                u64 flag = 1ull << idx;
                bx existAlready = (ctx->redArgsBitField & flag) == flag;
                if (existAlready && !(opt->flags & arg_flag_duplicatesAllowed)) {
                    // duplicated argument
                    return arg_error_duplicate;
                }
                ctx->redArgsBitField |= flag;
                if (opt->flags & arg_flag_requiredValue) {
                    if (value.size == 0) {
                        // missing required value
                        return arg_error_missingValue;
                    }
                }
                switch (opt->type) {
                    case arg_optType_string: break;
                    case arg_optType_f32: {
                        u64 offset = str_parseF32N(value, &ctx->foundF32Value);
                        if (offset != (value.size - 1)) {
                            return arg_error_invalidValue;
                        }
                    } break;
                    case arg_optType_i32: {
                        i64 outVal = 0;
                        u64 offset = str_parseS64N(value, &outVal);
                        if (offset != (value.size - 1)) {
                            return arg_error_invalidValue;
                        }
                    } break;
                    case arg_optType_flags: {
                        u64 offset = str_parseU32N(value, &ctx->foundFlagsValue);
                        if (offset != (value.size - 1)) {
                            return arg_error_invalidValue;
                        }
                    } break;
                    default: break;
                }
                return opt->shortName;
            }
        }
        ctx->nextIdx++;
    }

    // search for missing arguments
    for (u32 idx = ctx->nextIdx - ctx->inputOptsCount; (ctx->nextIdx - ctx->inputOptsCount) < ctx->optCount; ctx->inputOptsCount++) {
        arg_Opt* opt = ctx->opts + (ctx->nextIdx - ctx->inputOptsCount);
            if (opt->flags & arg_flag_required) {
            u64 flag = idx << 1ull;
            bx existAlready = (ctx->redArgsBitField & flag) == flag;
            if (!existAlready) {
                // duplicated argument
                return arg_error_missingArg;
            }
        }
    }

    ctx->foundKey        = str8("");
    ctx->foundValue      = str8("");
    ctx->foundi32Value   = 0;
    ctx->foundF32Value   = 0;
    ctx->foundFlagsValue = 0;
    ctx->currentOpt = NULL;
    return arg_end;
}