#ifndef _BASE_STR_
#define _BASE_STR_
#ifndef API
#define API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdarg.h>

static Str8 STR_TERMINATOR = {(u8*) "\0", 1};
static Str8 STR_EMPTY = {(u8*) "", 0};
static Str8 STR_NULL = {NULL, 0};

#if 0
// reference: https://github.com/mattiasgustavsson/libs/blob/main/strpool.h

typedef struct str_Pool {
    Arena* arena;
    str_handle* handles;
} str_Pool;

#define str_handleEqual(A, B) ((A).id == (B).id)

API void str_poolInit(Arena* arena, str_Pool* pool);
API str_handle str_poolInject(str_Pool* pool, Str8 str);
API Str8 str_poolGet(str_Pool* pool, str_handle handle);
#endif


#define str8(STR) str_makeViewSized((u8*) (STR), sizeof(STR) - 1)
#define s8(STR) str_makeViewSized((u8*) (STR), sizeof(STR) - 1)
INLINE Str8 str_makeViewSized(u8* c, u64 size) {
    Str8 str;
    str.size = size;
    str.content = c;
    return str;
}
#define str_fromCppStd(STDSTRING) str_makeViewSized((u8*) (STDSTRING).c_str(), (STDSTRING).size())

#define STRINGIFY(...) #__VA_ARGS__
#define str_cpToStaticNullTerminated(STR, STATSTR) for (u32 i = 0; i < (STR).size) (STATSTR)[i] = (STR).content[i]; (STATSTR)[(STR).size] = '\0'
#define str_isEmpty(STR) ((STR).size == 0)

INLINE bx str_isNullTerminated(Str8 str) {
    return str.content[str.size - 1] == '\0';
}

API Str8 str_makeSized(Arena* arena, u8* arr, u32 size);
API Str8 str_alloc(Arena* arena, mms size);
API Str8 str_copy(Arena* arena, Str8 sourceStr);

API Str8 str_copyNullTerminated(Arena* arena, Str8 str);


API void str_toLowerAscii(Str8 str);
API void str_toUpperAscii(Str8 str);
API Str8 str_subStr(Str8 str, u64 start, u64 size);
API Str8 str_from(Str8 str, u64 from);
API Str8 str_to(Str8 str, u64 to);

// returns the ut8 length of the string
API u64  str_length(Str8 str);

API f32  str_parseF32 (Str8 str);
API u64  str_parseF32N(Str8 str, f32* f32);
API i64  str_parseS64 (Str8 str);
API u64  str_parseS64N(Str8 str, i64* i64);
API u32  str_parseU32 (Str8 str);
API u64  str_parseU32N(Str8 str, u32* u32);
API Str8 str_parseQuotedStr(Str8 str);


API i64  str_find(Str8 str, Str8 findExp);
API i64  str_findChar(Str8 str, char c);
API i64  str_lastIndexOfChar(Str8 str, char c);
API bx str_hasPrefix(Str8 str, Str8 prefix);
API bx   str_hasSuffix(Str8 str, Str8 endsWith);
API bool str_startsWithChar(Str8 str, char startChar);
API bool str_isEqual(Str8 left, Str8 right);
API bool str_isWhiteSpace(Str8 str);
API i64  str_firstNonWhiteSpace(Str8 str);
API Str8 str_skipWhiteSpace(Str8 str);

API bool str_isWhitespaceChar(char c);


API bool str_isSpacingChar(char c);
API bool str_isNumberChar(char c);
API bool str_isEndOfLineChar(char c);
API bool str_isAlphaChar(char c);
API bool str_isEndOfLineChar(char c);


#pragma mark - String manipulation

API u64 str_containsSubStringCount(Str8 str, Str8 findStr);
API u64 str_findFirst(Str8 str, Str8 findStr, u64 offset);
API Str8 str_replaceAll(Arena* arena, Str8 str, Str8 replaceStr, Str8 replacement);

#pragma mark - UTF-8 functions

API u64 str_utf8Count(Str8 str);

typedef struct str_StringDecode {
    u32 codepoint;
    u32 size;
} str_StringDecode;

API str_StringDecode str_decodeUtf8(u8* str, u64 cap);

// limited to 15 fraction digits
// Store value needs to b at least 15+fracDigits+2 in size
API Str8 str_floatToStr(f64 value, Str8 storeStr, i32* decimalPos, i32 fracDigits);
API Str8 str_u32ToHex(Arena* arena, u32 value);

API Str8  str_fromCharPtr(u8* str, u64 size);
API Str8  str_fromNullTerminatedCharPtr(char* str);


#pragma mark - Utf8 to Utf16

API S16 str_toS16(Arena* arena, Str8 str);
API S32 str_toS32(Arena *arena, Str8 str);
API Str8  str_fromS16(Arena* arena, S16 str);


#pragma mark - hash

API u32 str_hash32(Str8 str);
API u64 str_hash64(Str8 str);

////////////////////////////
// NOTE(pjako): fmt parser... do we need that?

typedef enum str_FmtToken {
    str_fmtToken_unknown = 0,
    str_fmtToken_string  = 1,
    str_fmtToken_fmt     = 2,
} str_FmtToken;

typedef struct str_FmtParser {
    Str8         str;
    u32          offset;
    str_FmtToken currentToken;
    Str8         key;
    Str8         value;
    u32          blockIndex;
} str_FmtParser;

////////////////////////////
// NOTE(pjako): str8 format

typedef enum str_argType {
    str_argType_custom,
    str_argType_str,
    str_argType_char,
    str_argType_f32,
    str_argType_f64,
    str_argType_u32,
    str_argType_u64,
    str_argType_i32,
    str_argType_i64,
} str_argType;

typedef struct str_CustomVal {
    void* usrPtr;
    Str8 (*buildStrFn)(Arena* recordArena, Str8 fmtInfo, void* userPtr);
} str_CustomVal;

typedef struct str_Value {
    str_argType type;
    union {
        Str8 strVal;
        char charVal;
        f32 f32Val;
        f64 f64Val;
        u32 u32Val;
        u64 u64Val;
        i32 i32Val;
        i64 i64Val;
        str_CustomVal customVal;
    };
} str_Value;

typedef struct str_KeyValue {
    Str8 key;
    str_Value value;
} str_KeyValue;

#define str_keyValue(KEY, VALUE) str_kv(KEY, VALUE)
#define str_kv(KEY, VALUE) str__keyValue(str__convertToKey(KEY), str__convertToValue(VALUE))
INLINE str_KeyValue str__keyValue(Str8 key, str_Value value) {
    str_KeyValue kv;
    kv.key = key;
    kv.value = value;
    return kv;
}

// Usage: str8_fmt("Foo {}", "Bar") => "Foo Bar"
//#define str_fmt(ARENA, TEMPLATE, ...) STR_ARG_OVER_UNDER_FLOW_CHECKER(str8_fmtRaw, STR_AT_LEAST_TWO_ARGS, __VA_ARGS__)(ARENA, str__convertToValue(TEMPLATE), STR_ARR_MACRO_CHOOSER(__VA_ARGS__)(str_Arg, str_convertToAny, __VA_ARGS__))

// Usage: str8_join("Foo", " ", "Bar") => "Foo Bar"

#define str_fmt(ARENA, FMTSTR, ... ) STR_ARG_OVER_UNDER_FLOW_CHECKER(str_fmtRaw, STR_AT_LEAST_TWO_ARGS, __VA_ARGS__)(ARENA, str__convertToKey(FMTSTR), STR_ARR_MACRO_CHOOSER(__VA_ARGS__)(str_KeyValue, str__convertToKeyValue, __VA_ARGS__))

#define str_join(ARENA, ...) STR_ARG_OVER_UNDER_FLOW_CHECKER(str_joinRaw, STR_AT_LEAST_TWO_ARGS, __VA_ARGS__)(ARENA, STR_ARR_MACRO_CHOOSER(__VA_ARGS__)(str_KeyVakue, str__convertToValue, __VA_ARGS__))

API Str8 str_joinRaw(Arena* arena, u32 argCount, ...);
API Str8 str_joinVargs(Arena* arena, u32 argCount, va_list list);
API Str8 str_fmtRaw(Arena* arena, Str8 fmt, u32 argCount, ...);
API Str8 str_fmtVargs(Arena* arena, Str8 fmt, u32 argCount, va_list list);


#define str_record(STR, ARENA) for (u64 startIdx = mem_getArenaMemOffsetPos(ARENA) + 1;startIdx != 0; (( (startIdx - 1) < mem_getArenaMemOffsetPos(ARENA) ? (STR.content = &(ARENA)->memory[startIdx - 1], STR.size = (mem_getArenaMemOffsetPos(ARENA) - (startIdx - 1))) : (STR.content = NULL, STR.size = 0)  ), startIdx = 0))

////////////////////////////
// NOTE(pjako): fmt/join implementation

#define STR(s)  #s

#ifdef __cplusplus
#define STR_TOFN(TYPE) str__toVal
#define STR_TOKEY(TYPE) str__toKey
#define STR_TOKV(TYPE) str__toKeyVal

#define str__convertToKeyValue(V) str__toKeyVal(V, sizeof(V), STR(V), sizeof(STR(V)))
#define str__convertToKey(V) str__toKey(V, sizeof(V), STR(V), sizeof(STR(V)))
// Never change V to anything more then a single letter since this cause bugs when this expression is passed: " "
#define str__convertToValue(V) str__toVal(V, sizeof(V), STR(V), sizeof(STR(V)))

#define STR_ARR_PREFIX(TYPE, COUNT) COUNT, 
#define STR_ARR_POSTFIX(COUNT) 
//#define STR_ARR_PREFIX(TYPE, COUNT) TYPE str__args##__COUNT__[COUNT] =
//#define STR_ARR_POSTFIX(COUNT) ;



#else

#define STR_TOKV(TYPE) str__##TYPE##ToKeyVal
#define str__convertToKeyValue(V) _Generic(V, \
    str_KeyValue: STR_TOKV(kv), \
    Str8: STR_TOKV(str8), \
    char: STR_TOKV(char), \
    u8: STR_TOKV(uchar), \
    str_CustomVal: STR_TOKV(custom), \
    u32: STR_TOKV(u32), \
    u64: STR_TOKV(u64), \
    i32: STR_TOKV(i32), \
    i64: STR_TOKV(i64), \
    f32: STR_TOKV(f32), \
    f64: STR_TOKV(f64), \
    default: STR_TOKV(custom)  \
)((V), sizeof(V), STR(V), sizeof(STR(V)))

#define STR_TOKEY(TYPE) str__##TYPE##ToKey
#define str__convertToKey(V) _Generic(V, \
    Str8: STR_TOKEY(str8), \
    default: STR_TOKEY(str8)  \
)((V), sizeof(V), STR(V), sizeof(STR(V)))

#define STR_TOFN(TYPE) str__##TYPE##ToVal
// Never change V to anything more then a single letter since this cause bugs when this expression is passed: " "

#define str___convertToValue(V) _Generic(V, \
    Str8: STR_TOFN(str8), \
    char: STR_TOFN(char), \
    u8: STR_TOFN(uchar), \
    str_CustomVal: STR_TOFN(custom), \
    u32: STR_TOFN(u32), \
    u64: STR_TOFN(u64), \
    i32: STR_TOFN(i32), \
    i64: STR_TOFN(i64), \
    f32: STR_TOFN(f32), \
    f64: STR_TOFN(f64), \
    default: STR_TOFN(custom)  \
)

#define str__convertToValue(V) (str___convertToValue(V))((V), sizeof(V), STR(V), sizeof(STR(V)))
#define STR_ARR_PREFIX(TYPE, COUNT) COUNT, 
#define STR_ARR_POSTFIX(COUNT) 
//#define STR_ARR_PREFIX(TYPE, COUNT) ((TYPE*) (&(TYPE[COUNT]) 
//#define STR_ARR_POSTFIX(COUNT) )), (COUNT)
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

INLINE bx str__readNumExpression(str_Value* out, const char* strExpression, u32 strExpressionSize) {
    if ((strExpression[0] < '0' ||  strExpression[0] > '9') && strExpression[0] != '-') return false;
    if (strExpression[1] == '\0') {
        // single digit
        Str8 str;
        str.size = 1;
        str.content = (u8*) strExpression;
        out->type = str_argType_str;
        out->strVal = str;
        return true;
    }
    if (strExpression[1] < '0' ||  strExpression[1] > '9') return false;
    u32 idx = 1;
    for (; idx < (strExpressionSize - 1); idx++) {
        char c = strExpression[idx];
        if (c >= '0' && c <= '9') continue;
        if (c != 'u' && c != 'U' && c != 'l' && c != 'L' && c != 'z' && c != 'Z') return false;
        break;
    }

    // remove the ending of the number 23u  23uul ect.

    Str8 str;
    str.size = idx;
    str.content = (u8*) strExpression;
    out->type = str_argType_str;
    out->strVal = str;
    return true;
}

////////////////////////////
// NOTE(pjako): Convert "anything" to str_Value

INLINE str_Value str_valToVal(str_Value val, u32 size, const char* strExpression, u32 strExpressionSize) {
    return val;
}

INLINE str_Value STR_TOFN(custom)(str_CustomVal custom, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_custom;
    value.customVal = custom;
    return value;
}

INLINE str_Value STR_TOFN(char)(char c, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_char;
    value.charVal = c;
    return value;
}

INLINE str_Value STR_TOFN(uchar)(unsigned char c, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_char;
    value.charVal = c;
    return value;
}

INLINE str_Value STR_TOFN(str8)(Str8 str, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_str;
    value.strVal = str;
    return value;
}

INLINE str_Value STR_TOFN(f32)(f32 floatVal, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_f32;
    value.f32Val = floatVal;
    return value;
}

INLINE str_Value STR_TOFN(f64)(f64 floatVal, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if ((strExpression[0] >= '0' && strExpression[0] <= '9') || strExpression[0] == '-' || strExpression[0] == '.') {
        bool simpleFloat = true;
        u32 idx = (strExpression[0] == '-' || strExpression[0] == '.') ? 1 : 0;
        for (; idx < (strExpressionSize - 1); idx++) {
            if ( (strExpression[idx] >= '0' && strExpression[idx] <= '9') || strExpression[idx] == '.') continue;
            simpleFloat = false;
            break;
        }
        if (simpleFloat) {
            // pass simple float expression as string
            Str8 str;
            str.size = strExpressionSize - 1;
            str.content = (u8*) strExpression;
            value.type = str_argType_str;
            value.strVal = str;
            return value;
        }
    }
    value.type = str_argType_f64;
    value.f64Val = floatVal;
    return value;
}

INLINE str_Value STR_TOFN(u32)(u32 u32Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return value;
    value.type = str_argType_u32;
    value.u32Val = u32Val;
    return value;
}

INLINE str_Value STR_TOFN(u64)(u64 u64Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return value;
    value.type = str_argType_u64;
    value.u64Val = u64Val;
    return value;
}

INLINE str_Value STR_TOFN(i32)(i32 i32Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (strExpression[0] == '\'') {
        // handle char literals
        value.type = str_argType_char;
        value.charVal = (char) (maxVal(minVal(255, i32Val), 0));
        return value;
    }
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return value;
    value.type = str_argType_i32;
    value.i32Val = i32Val;
    return value;
}

INLINE str_Value STR_TOFN(i64)(i64 i64Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return value;
    value.type = str_argType_i64;
    value.i64Val = i64Val;
    return value;
}


////////////////////////////
// NOTE(pjako): Convert char* (and Str8) to Str8 to

INLINE Str8 STR_TOKEY(str8)(Str8 str, u32 size, const char* strExpression, u32 strExpressionSize) {
    return str;
}

INLINE Str8 STR_TOKEY(charConst)(const char* val, u32 size, const char* strExpression, u32 strExpressionSize) {
    Str8 str;
    if (strExpression[0] == '"') { // if expression starts with '"' it is a string literal
        // its a string literal
        str.size = size - 1;
        str.content = (u8*) val;
    } else if (strExpression[0] != '\0' && strExpression[1] != '\0' && strExpression[2] != '\0' && strExpression[0] != 'u' && strExpression[1] != '8' && strExpression[2] != '"') {
        // its a string literal starting with u8"
        str.size = size - 1;
        str.content = (u8*) val;
    } else {
        str.size = strlen(val);
        str.content = (u8*) val;
    }

    return str;
}

////////////////////////////
// NOTE(pjako): Convert to str_KeyValue

INLINE str_KeyValue STR_TOKV(value)(str_Value value) {
    str_KeyValue keyValue;
    keyValue.key.content = NULL;
    keyValue.key.size = 0;
    keyValue.value = value;

    return keyValue;
}

INLINE str_KeyValue STR_TOKV(kv)(str_KeyValue kv, u32 size, const char* strExpression, u32 strExpressionSize) {
    return kv;
}

INLINE str_KeyValue STR_TOKV(charConst)(const char* val, u32 size, const char* strExpression, u32 strExpressionSize) {
    Str8 str;
    if (strExpression[0] == '"') { // if expression starts with '"' it is a string literal
        // its a string literal
        str.size = size - 1;
        str.content = (u8*) val;
    } else if (strExpression[0] != '\0' && strExpression[1] != '\0' && strExpression[2] != '\0' && strExpression[0] != 'u' && strExpression[1] != '8' && strExpression[2] != '"') {
        // its a string literal starting with u8"
        str.size = size - 1;
        str.content = (u8*) val;
    } else {
        str.size = strlen(val);
        str.content = (u8*) val;
    }

    str_Value value;
    value.type = str_argType_str;
    value.strVal = str;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(custom)(str_CustomVal custom, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_custom;
    value.customVal = custom;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(char)(char c, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_char;
    value.charVal = c;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(uchar)(unsigned char c, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_char;
    value.charVal = c;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(str8)(Str8 str, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_str;
    value.strVal = str;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(f32)(f32 floatVal, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    value.type = str_argType_f32;
    value.f32Val = floatVal;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(f64)(f64 floatVal, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if ((strExpression[0] >= '0' && strExpression[0] <= '9') || strExpression[0] == '-' || strExpression[0] == '.') {
        bool simpleFloat = true;
        u32 idx = (strExpression[0] == '-' || strExpression[0] == '.') ? 1 : 0;
        for (; idx < (strExpressionSize - 1); idx++) {
            if ( (strExpression[idx] >= '0' && strExpression[idx] <= '9') || strExpression[idx] == '.') continue;
            simpleFloat = false;
            break;
        }
        if (simpleFloat) {
            // pass simple float expression as string
            Str8 str;
            str.size = strExpressionSize - 1;
            str.content = (u8*) strExpression;
            value.type = str_argType_str;
            value.strVal = str;
            return STR_TOKV(value)(value);
        }
    }
    value.type = str_argType_f64;
    value.f64Val = floatVal;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(u32)(u32 u32Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return STR_TOKV(value)(value);
    value.type = str_argType_u32;
    value.u32Val = u32Val;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(u64)(u64 u64Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return STR_TOKV(value)(value);
    value.type = str_argType_u64;
    value.u64Val = u64Val;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(i32)(i32 i32Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (strExpression[0] == '\'') {
        // handle char literals
        value.type = str_argType_char;
        value.charVal = (char) (maxVal(minVal(255, i32Val), 0));
        return STR_TOKV(value)(value);
    }
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return STR_TOKV(value)(value);
    value.type = str_argType_i32;
    value.i32Val = i32Val;
    return STR_TOKV(value)(value);
}

INLINE str_KeyValue STR_TOKV(i64)(i64 i64Val, u32 size, const char* strExpression, u32 strExpressionSize) {
    str_Value value;
    if (str__readNumExpression(&value, strExpression, strExpressionSize)) return STR_TOKV(value)(value);
    value.type = str_argType_i64;
    value.i64Val = i64Val;
    return STR_TOKV(value)(value);
}


////////////////////////////
// NOTE(pjako): Macros to handle variable length arguments and conversion

#define STR_AT_LEAST_TWO_ARGS(...) STATIC_ASSERT("Needs at least two arguments!");
#define STR_AT_LEAST_ONE_ARG(...) STATIC_ASSERT("Needs at least one argument!");

#define STR__TOO_MANY(...) STATIC_ASSERT("A maximum of 12 dynamic arguments are currently supported")
#define STR__ARR12(TYPE, CONV, a, b, c, d, e, f, g, h, i, j, k, l, ...) STR_ARR_PREFIX(TYPE, 12) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g), CONV(h), CONV(i), CONV(j), CONV(k), CONV(l) STR_ARR_POSTFIX(12)
#define STR__ARR11(TYPE, CONV, a, b, c, d, e, f, g, h, i, j, k) STR_ARR_PREFIX(TYPE, 11) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g), CONV(h), CONV(i), CONV(j), CONV(k) STR_ARR_POSTFIX(11)
#define STR__ARR10(TYPE, CONV, a, b, c, d, e, f, g, h, i, j) STR_ARR_PREFIX(TYPE, 10) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g), CONV(h), CONV(i), CONV(j) STR_ARR_POSTFIX(10)
#define STR__ARR9(TYPE, CONV, a, b, c, d, e, f, g, h, i) STR_ARR_PREFIX(TYPE, 9) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g), CONV(h), CONV(i) STR_ARR_POSTFIX(9)
#define STR__ARR8(TYPE, CONV, a, b, c, d, e, f, g, h) STR_ARR_PREFIX(TYPE, 8) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g), CONV(h) STR_ARR_POSTFIX(8)
#define STR__ARR7(TYPE, CONV, a, b, c, d, e, f, g) STR_ARR_PREFIX(TYPE, 7) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f), CONV(g) STR_ARR_POSTFIX(7)
#define STR__ARR6(TYPE, CONV, a, b, c, d, e, f) STR_ARR_PREFIX(TYPE, 6) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e), CONV(f) STR_ARR_POSTFIX(6)
#define STR__ARR5(TYPE, CONV, a, b, c, d, e) STR_ARR_PREFIX(TYPE, 5) CONV(a), CONV(b), CONV(c), CONV(d), CONV(e) STR_ARR_POSTFIX(5)
#define STR__ARR4(TYPE, CONV, a, b, c, d) STR_ARR_PREFIX(TYPE, 4) CONV(a), CONV(b), CONV(c), CONV(d) STR_ARR_POSTFIX(4)
#define STR__ARR3(TYPE, CONV, a, b, c) STR_ARR_PREFIX(TYPE, 3) CONV(a), CONV(b), CONV(c) STR_ARR_POSTFIX(3)
#define STR__ARR2(TYPE, CONV, a, b) STR_ARR_PREFIX(TYPE, 2)  CONV(a), CONV(b) STR_ARR_POSTFIX(2)
#define STR__ARR1(TYPE, CONV, a) STR_ARR_PREFIX(TYPE, 1) CONV(a) STR_ARR_POSTFIX(1)
#define STR__ARR0(...) 0
#define STR__ARG0(_0, ...) _0

#define STR__FUNC_CHOOSER(_f1, _f2, _f3, _f4, _f5, _f6, _f7, _f8, _f9, _f10, _f11, _f12, _f13, _f14, _f15, _f16, ...) _f16
#define STR__FUNC_RECOMPOSER(argsWithParentheses) STR__FUNC_CHOOSER argsWithParentheses
#define STR__CHOOSE_FROM_ARG_COUNT(...) STR__FUNC_RECOMPOSER((__VA_ARGS__, STR__TOO_MANY, STR__TOO_MANY, STR__TOO_MANY, STR__ARR12, STR__ARR11, STR__ARR10, STR__ARR9, STR__ARR8, STR__ARR7, STR__ARR6, STR__ARR5, STR__ARR4, STR__ARR3, STR__ARR2, STR__ARR1))

#define STR__NO_ARG_EXPANDER(...) ,,,,,,,,,,,,,,, STR__ARG0(__VA_ARGS__)

#define STR__CHOOSE_FROM_ARG_COUNT_CHECK(FN, ...) STR__FUNC_RECOMPOSER((__VA_ARGS__, STR__TOO_MANY, STR__TOO_MANY, STR__TOO_MANY, FN, FN, FN, FN,  FN, FN, FN, FN, FN, FN, FN, FN))

/* collects all passed arguments chooses the STR__ARRN that can fit all of them in */
#define STR_ARR_MACRO_CHOOSER(...) STR__CHOOSE_FROM_ARG_COUNT(STR__NO_ARG_EXPANDER __VA_ARGS__ (STR__ARR0))

/* returns a static assert if there are too many args, calls FNNOARGS if there are too little */
#define STR_ARG_OVER_UNDER_FLOW_CHECKER(FN, FNNOARGS, ...) STR__CHOOSE_FROM_ARG_COUNT_CHECK(FN, STR__NO_ARG_EXPANDER __VA_ARGS__ (FNNOARGS))

#endif /* _BASE_STR_ */