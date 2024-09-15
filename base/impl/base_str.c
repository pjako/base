#include "base/base.h"
#include "base/base_types.h"
#include "base/base_math.h"
#include "base/base_mem.h"
#include "base/base_str.h"

S8 str_makeSized(Arena* arena, u8* arr, u32 size) {
   S8 str;
   str.size = size;
   str.content = mem_arenaPushArray(arena, u8, size);
   mem_copy(str.content, arr, str.size);
   return str;
}

i64  str_find(S8 str, S8 findExp) {
    for (u64 idx = 0; (idx + findExp.size)  < str.size; idx++) {
        for (u64 fi = 0; str.content[idx + fi] != str.content[fi]; fi++) {
            continue;
        }
        return idx;
    }
    return -1;
}

i64  str_findChar(S8 str, char c) {
    for (u64 idx = 0; idx < str.size; idx++) {
        if (str.content[idx] == c) {
            return idx;
        }
    }
    return -1;
}
API i64 str_lastIndexOfChar(S8 str, char c) {
   for (i64 idx = i64_cast(str.size) - 1; idx >= 0; idx--) {
      if (str.content[idx] == c) {
         return idx;
      }
   }
   return -1;
}

bool str_startsWithChar(S8 str, char startChar) {
    if (str.size == 0 || str.content[0] != startChar) return false;
    return true;
}

bx str_hasPrefix(S8 str, S8 prefix) {
   if (prefix.size > str.size) {
      return false;
   }

   for (u64 idx = 0; prefix.size > idx; idx++) {
      if (str.content[idx] != prefix.content[idx]) return false;
   }

   return true;
}

bx str_hasSuffix(S8 str, S8 endsWith) {
   if (str.size < endsWith.size) return false;
   u8* l = str.content + (str.size - endsWith.size);
   u8* r = endsWith.content;

   for (u64 idx = 0; endsWith.size > idx; idx++) {
      if (l[idx] != r[idx]) return false;
   }

   return true;
}

S8 str_toLowerAscii(S8 str) {
    for (u64 idx = 0; str.size > idx; idx++) {
        char c = str.content[idx];
        if (c >= 'A' && c <= 'Z') {
            str.content[idx] = 'a' + (c - 'A');
        }
    }
    return str;
}

S8 str_toUpperAscii(S8 str) {
    for (u64 idx = 0; str.size > idx; idx++) {
        char c = str.content[idx];
        if (c >= 'a' && c <= 'Z') {
            str.content[idx] = 'A' + (c - 'a');
        }
    }
    return str;
}

S8 str_subStr(S8 str, u64 start, u64 size) {
    if (str.size == 0) {
        return str;
    }
    if (str.size <= start) {
        return STR_NULL;
    }

    S8 subStr;
    subStr.content = str.content + start;
    subStr.size = minVal(str.size - start, size);
    return subStr;
}

API S8 str_from(S8 str, u64 from) {
   if (from >= str.size) {
      //ASSERT(!"start out of str range");
      return STR_NULL;
   }
   u64 start = minVal(str.size - 1, from);
   S8 result;
   result.content = &str.content[start];
   result.size = str.size - start;
   return result;
}

API S8 str_to(S8 str, u64 to) {
   str.size = minVal(to, str.size);
   return str;
}

u64 str_length(S8 str) {
   char* s = (char*) str.content;
   const char* t = (const char*) str.content;
   u64 length = 0;

   while ((u64)(s - t) < str.size && '\0' != *s) {
      if (0xf0 == (0xf8 & *s)) {
         /* 4-byte utf8 code point (began with 0b11110xxx) */
         s += 4;
      } else if (0xe0 == (0xf0 & *s)) {
         /* 3-byte utf8 code point (began with 0b1110xxxx) */
         s += 3;
      } else if (0xc0 == (0xe0 & *s)) {
         /* 2-byte utf8 code point (began with 0b110xxxxx) */
         s += 2;
      } else { /* if (0x00 == (0x80 & *s)) { */
         /* 1-byte ascii (began with 0b0xxxxxxx) */
         s += 1;
      }

      /* no matter the bytes we marched s forward by, it was
      * only 1 utf8 codepoint */
      length++;
   }

   if ((u64)(s - t) > str.size) {
      length--;
   }
   return length;
}

bool str_isEqual(S8 left, S8 right) {
    if (left.size != right.size) return false;
    for (u64 idx = 0; idx < left.size; idx++) {
      if (left.content[idx] != right.content[idx]) {
         return false;
      }
    }
    return true;
}

bool str_isSpacingChar(char c) {
    return ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f'));
}

bool str_isNumberChar(char c) {
    return ((c >= '0') && (c <= '9'));
}

bool str_isEndOfLineChar(char c) {
    return ((c == '\n') || (c == '\r'));
}

bool str_isAlphaChar(char c) {
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

f32  str_parseF32 (S8 str) {
   f32 val = 0.f;
   str_parseF32N(str, &val);
   return val;
}

u64  str_parseF32N(S8 str, f32* f) {
   i64 left = 0;
   u64 offset = str_parseS64N(str, &left);
   if (offset == 0) {
      *f = 0;
      return 0;
   }
   *f = f32_cast(left);
   if (str.size > (offset + 2) && str.content[offset + 1] == '.') {
      S8 sub = str_subStr(str, offset + 1, str.size);
      i64 right = 0;
      u64 offsetRight = str_parseS64N(sub, &right);
      if (offsetRight != 0 && right > 0) {
         *f = (left > 0 ? 1.f : -1.f) * f32_cast(right) / f32_cast(offsetRight);
         offset += offsetRight;
      }
   }
   return offset;
}

i64  str_parseS64 (S8 str) {
   i64 val = 0;
   str_parseS64N(str, &val);
   return val;
}

u64  str_parseS64N(S8 str, i64* s) {
   u64 out = 0;
   u64 idx = 1;
   if (str.size == 0) {
      return 0;
   }
   i64 mult = str.content[0] == '-' ? -1 : 1;

   for (; str.size > idx; idx++) {
      char numChar = str.content[idx];
      if (numChar < '0' || numChar > '9') {
         *s = out * mult;
         return idx;
      }
      out *= 10;
      out += numChar - '0';
      if (out >= i64_max) {
         *s = mult * i64_max;
         return idx;
      }
   }
   *s = out * mult;
   return idx;

}

u32  str_parseU32 (S8 str) {
   u32 u = 0;
   str_parseU32N(str, &u);
   return u;
}

u64 str_parseU32N(S8 str, u32* u) {
   u64 out = 0;
   u64 idx = 0;
   for (; str.size > idx; idx++) {
      char numChar = str.content[idx];
      if (numChar < '0' || numChar > '9') {
         *u = out;
         return idx;
      }
      out *= 10;
      out += numChar - '0';
      if (out >= u32_max) {
         *u = u32_max;
         return idx;
      }
   }
   *u = out;
   return idx;
}

S8 str_copyNullTerminated(Arena* arena, S8 str) {
   mms size = str_isNullTerminated(str) ? str.size : str.size + 1;
   S8 result = {
      .size = size,
      .content = (u8*) mem_arenaPush(arena, size)
   };
   mem_copy(result.content, str.content, str.size);
   result.content[result.size - 1] = '\0';

   return result;
}

S8 str_alloc(Arena* arena, mms size) {
   S8 result = {
      .size = size,
      .content = (u8*) mem_arenaPush(arena, size)
   };

   return result;
}

S8 str_copy(Arena* arena, S8 sourceStr) {
   if (sourceStr.size == 0) {
      return STR_NULL;
   }
   S8 result = str_alloc(arena, sourceStr.size);
   mem_copy(result.content, sourceStr.content, sourceStr.size);

   return result;
}

u64 str_findFirst(S8 str, S8 findStr, u64 offset) {
   u64 i = 0;
   if (findStr.size > 0) {
      i = str.size;
      if (str.size >= findStr.size) {
         i = offset;
         i8 c = findStr.content[0];
         u64 onePastLast = str.size - findStr.size + 1;
         for (; i < onePastLast; i++) {
            if (str.content[i] == c) {
               if ((str.size - i) >= findStr.size) {
                  S8 sub = {.content = str.content + i, .size = findStr.size};
                  if (str_isEqual(sub, findStr)) {
                     break;
                  }
               }
            }
         }
         if (i == onePastLast) {
            i = str.size;
         }
      }
   }
   return i;
}

u64 str_findLast(S8 str, S8 findStr, u64 offset) {
   if ((findStr.size + offset) > str.size) {
      return str.size;
   }
   u64 notFoundReturn = str.size;
   str = str_from(str, offset);
   for (u32 idx = str.size - (findStr.size); idx != 0; idx--) {
      if (str_isEqual(str_subStr(str, idx, findStr.size), findStr)) {
         return idx;
      }
   }
   return notFoundReturn;
}

u64 str_containsSubStringCount(S8 str, S8 findStr) {
   u64 ct = 0;
   u64 idx = 0;
   while (true) {
      idx = str_findFirst(str, findStr, idx);
      if (idx == str.size) {
         break;
      }

      ct++;
      idx++;
   }
   return ct;
}

S8 str_replaceAll(Arena* arena, S8 str, S8 replaceStr, S8 replacement) {
   if (replaceStr.size == 0) return str;
   u64 replaceable = str_containsSubStringCount(str, replaceStr);
   if (replaceable == 0) return str;

   u64 new_size = (str.size - replaceable * replaceStr.size) + (replaceable * replacement.size);
   S8 ret = str_alloc(arena, new_size);

   b8 replaced;
   u64 o = 0;
   for (u64 i = 0; i < str.size;) {
      replaced = false;
      if (str.content[i] == replaceStr.content[0]) {
         if ((str.size - i) >= replaceStr.size) {
               S8 sub = { .content = str.content + i, .size = replaceStr.size };
               if (str_isEqual(sub, replaceStr)) {
                  // replace this one
                  memmove(ret.content + o, replacement.content, replacement.size);
                  replaced = true;
               }
         }
      }
      
      if (replaced) {
         o += replacement.size;
         i += replaceStr.size;
         continue;
      }
      
      ret.content[o] = str.content[i];
      o++; i++;
   }

   return ret;
}

///////////////////////////////////////
// UTF-8 functions

u64 str_utf8Count(S8 str) {
    u64 length = 0;
    u64 idx = 0;
    while (idx < str.size) {
        if (0xf0 == (0xf8 & str.content[idx])) {
            // 4-byte utf8 code point (began with 0b11110xxx)
            idx += 4;
        } else if (0xe0 == (0xf0 & str.content[idx])) {
            // 3-byte utf8 code point (began with 0b1110xxxx)
            idx += 3;
        } else if (0xc0 == (0xe0 & str.content[idx])) {
            // 2-byte utf8 code point (began with 0b110xxxxx)
            idx += 2;
        } else {
            // 1-byte ascii (began with 0b0xxxxxxx)
            idx += 1;
        }

        // it is always only one rune independent of its size
        length++;
    }

    if (idx > str.size) {
        // rune size goes beyond content buffer so we go back on that one
        length--;
    }
    return length;
}

////////////////////////////
// NOTE(pjako): hash

API u32 str_hash32(S8 str) {
    u32 hash = 5381;
    i32 c;
    for (u64 i = 0; i < str.size; i++) {
        hash = ((hash << 5) + hash) + str.content[i]; /* hash * 33 + c */
    }
    return hash;
}

API u64 str_hash64(S8 str) {
    u32 hash1 = 5381;
    u32 hash2 = 52711;
    u64 i = str.size;
    while (i--) {
        u8 c = str.content[i];
        hash1 = (hash1 * 33) ^ c;
        hash2 = (hash2 * 33) ^ c;
    }

    return (hash1 >> 0) * 4096 + (hash2 >> 0);
}

S8 str_fromCharPtr(u8* charArr, u64 size) {
   S8 str;
   str.content = charArr;
   str.size = size;

   return str;
}

S8 str_fromNullTerminatedCharPtr(char* charArr) {
   S8 str;
   str.content = (u8*) charArr;
   str.size = 0;

   for (;charArr[str.size] != '\0'; str.size++);

   return str;
}

#if 0
static str_Decode str_decodeUtf8(u8* str, u64 cap) {
   str_Decode result = {'#', 1};
   u16 x = str[0];
   if (x < 0xD800 || 0xDFFF < x) {
      result.codepoint = x;
   } else if (cap >= 2) {
      u16 y = str[1];
      if (0xD800 <= x && x < 0xDC00 && 0xDC00 <= y && y < 0xE000) {
         u16 xj = x - 0xD800;
         u16 yj = y - 0xDc00;
         u32 xy = (xj << 10) | yj;
         result.codepoint = xy + 0x10000;
         result.size = 2;
      }
   }
   return result;
}
#endif

str_StringDecode str_decodeUtf8(u8* str, u64 cap) {
   str_StringDecode result = {'#', 1};
   u16 x = str[0];
   if (x < 0xD800 || 0xDFFF < x){
      result.codepoint = x;
   } else if (cap >= 2){
      u16 y = str[1];
      if (0xD800 <= x && x < 0xDC00 &&
         0xDC00 <= y && y < 0xE000){
         u16 xj = x - 0xD800;
         u16 yj = y - 0xDc00;
         u32 xy = (xj << 10) | yj;
         result.codepoint = xy + 0x10000;
         result.size = 2;
      }
   }
   return(result);
}

static u32 str_encodeUtf16(u16* dst, u32 codepoint) {
   u32 size = 0;
   if (codepoint < 0x10000) {
      dst[0] = codepoint;
      #if OS_WIN == 1
      size = 1;
      #else
      size = 2;
      #endif
   } else {
      u32 cpj = codepoint - 0x10000;
      dst[0] = (cpj >> 10 ) + 0xD800;
      dst[1] = (cpj & 0x3FF) + 0xDC00;
      #if OS_WIN == 1
      size = 2;
      #else
      size = 4;
      #endif
   }
   return size;
}

str_StringDecode str_decodeUtf16(u16 *str, u32 cap) {
    str_StringDecode result = {'#', 1};
    u16 x = str[0];
    if (x < 0xD800 || 0xDFFF < x){
        result.codepoint = x;
    } else if (cap >= 2) {
        u16 y = str[1];
        if (0xD800 <= x && x < 0xDC00 &&
            0xDC00 <= y && y < 0xE000) {
            u16 xj = x - 0xD800;
            u16 yj = y - 0xDc00;
            u32 xy = (xj << 10) | yj;
            result.codepoint = xy + 0x10000;
            result.size = 2;
        }
    }
    return(result);
}

u32 str_encodeUtf8(u8 *dst, u32 codepoint){
    u32 size = 0;
    if (codepoint < (1 << 8)) {
        dst[0] = codepoint;
        size = 1;
    } else if (codepoint < (1 << 11)) {
        dst[0] = 0xC0 | (codepoint >> 6);
        dst[1] = 0x80 | (codepoint & 0x3F);
        size = 2;
    } else if (codepoint < (1 << 16)) {
        dst[0] = 0xE0 | (codepoint >> 12);
        dst[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[2] = 0x80 | (codepoint & 0x3F);
        size = 3;
    } else if (codepoint < (1 << 21)) {
        dst[0] = 0xF0 | (codepoint >> 18);
        dst[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dst[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[3] = 0x80 | (codepoint & 0x3F);
        size = 4;
    } else {
        dst[0] = '#';
        size = 1;
    }
    return(size);
}

S8 str_fromS16(Arena* arena, S16 str) {
   u8 *memory = mem_arenaPushArrayZero(arena, u8, str.size * 3 + 1);

   u8  *dptr = memory;
   u16 *ptr  = str.content;
   u16 *opl  = str.content + str.size;
   for (; ptr < opl;) {
      str_StringDecode decode = str_decodeUtf16(ptr, (u64)(opl - ptr));
      u16 enc_size = str_encodeUtf8(dptr, decode.codepoint);
      ptr += decode.size;
      dptr += enc_size;
   }

   dptr[0] = 0;
   //dptr[3] = 0;

   u64 allocCount = str.size * 3 + 1;
   u64 stringCount = (u64)(dptr - memory);
   u64 unusedCount = allocCount - stringCount - 1;
   //mem_arenaPopAmount(arena, unusedCount * sizeof(*memory));

   S8 result = {memory, stringCount};
   return(result);
}

S16 str_toS16(Arena *arena, S8 str) {
   u64 allocSize = str.size * 4 + 2;
   u16 *memory = mem_arenaPushArray(arena, u16, allocSize);

   u16 *dptr = memory;
   u8 *ptr = str.content;
   u8 *opl = str.content + str.size;
   u16* wstr = (u16*) memory;

   for (; ptr < opl;) {
      str_StringDecode decode = str_decodeUtf8(ptr, (u64)(opl - ptr));
      u32 encSize = str_encodeUtf16(dptr, decode.codepoint);
      ptr  = ptr + decode.size;
      dptr = &dptr[encSize];
   }

   *dptr = 0;
   dptr += 1;
   *dptr = 0;

   u64 stringCount = (u64)(dptr - memory);
   u64 unusedCount = allocSize - stringCount - 1;
   //mem_arenaPopAmount(arena, unusedCount);

   S16 result = {memory, stringCount};
   return result;
}

S32 str_toS32(Arena *arena, S8 str) {
   u64 allocSize = str.size * 4 + 2;
   u32 *memory = mem_arenaPushArray(arena, u32, allocSize);

   u32 *dptr = memory;
   u8 *ptr = str.content;
   u8 *opl = str.content + str.size;
   u32* wstr = (u32*) memory;

   for (; ptr < opl;) {
      str_StringDecode decode = str_decodeUtf8(ptr, (u64)(opl - ptr));
      ptr  = ptr + decode.size;
      *dptr = decode.codepoint;
      dptr = dptr + 1;
   }

   *dptr = 0;
   dptr += 1;
   *dptr = 0;

   u64 stringCount = (u64)(dptr - memory);
   u64 unusedCount = allocSize - stringCount - 1;
   //mem_arenaPopAmount(arena, unusedCount);

   S32 result = {memory, stringCount};
   return result;
}

////////////////////////////
// NOTE(pjako): fomat/join

LOCAL void str_recordStr(Arena* arena, S8 str) {
   mem_copy(mem_arenaPush(arena, str.size), str.content, str.size);
}

LOCAL void str_recordCustom(Arena* arena, str_CustomVal* customVal, S8 format) {
   customVal->buildStrFn(arena, format, customVal->usrPtr);
}

LOCAL i32 str__f32ToCharArr(char const** start, u32 *len, char* out, i32* decimal_pos, f64 value, i32 frac_digits);

LOCAL void str_recordf32_cast(Arena* arena, f32 floatVal) {
   u32 fracDigits = 10;
   const char* startChar = NULL;
   char str[512];
   u32 length = 0;
   i32 decPos = 0;
   i32 start = str__f32ToCharArr(&startChar, &length, str, &decPos, floatVal, fracDigits);
   if (decPos == 0) {
      mem_copy(mem_arenaPush(arena, 2), "0.", 2);
   }
   if (decPos < length) {
      mem_copy(mem_arenaPush(arena, decPos), startChar, decPos);
      mem_copy(mem_arenaPush(arena, 1),  ".", 1);
      mem_copy(mem_arenaPush(arena, length - decPos), &startChar[decPos], length - decPos);
   } else {
      mem_copy(mem_arenaPush(arena, length), &startChar[0], length);
   }
}

LOCAL void str_recordF64(Arena* arena, f64 floatVal) {
   u32 fracDigits = 15;
   const char* startChar = NULL;
   char str[512];
   u32 length = 0;
   i32 decPos = 0;
   i32 start = str__f32ToCharArr(&startChar, &length, &str[0], &decPos, floatVal, fracDigits);
   if (decPos == 0) {
      mem_copy(mem_arenaPush(arena, length), "0.", 2);
   }
   mem_copy(mem_arenaPush(arena, decPos), startChar, decPos);
   mem_copy(mem_arenaPush(arena, 1),  ".", 1);
   mem_copy(mem_arenaPush(arena, length - decPos), &startChar[decPos], length - decPos);
}
// TODO: consider fast coversation alternative: https://github.com/fmtlib/format-benchmark/blob/master/src/u2985907.h
LOCAL void str_recordU64(Arena* arena, u64 value, bx isNegativ, S8 format) {
   if (isNegativ) {
      u8* ptr = mem_arenaPush(arena, 1);
      ptr[0] = '-';
   }
   if (value == 0) {
      u8* ptr = mem_arenaPush(arena, 1);
      ptr[0] = '0';
      return;
   }
   u8 intStr[21];
   uint32_t pos = 20;
   for (uint64_t tmpVal = value; tmpVal > 0;tmpVal /= 10) {
      uint64_t digit = tmpVal - ((tmpVal / 10) * 10);
      intStr[pos--] = '0' + digit;
   }
   pos += 1;
   u32 length = 21 - pos;
   mem_copy(mem_arenaPush(arena, length), intStr + pos, length);
}

LOCAL i32 str__ufast_utoa10(u32 value, char* str);

LOCAL void str_recordU32(Arena* arena, u32 value, bx isNegativ, S8 format) {
   if (isNegativ) {
      u8* ptr = mem_arenaPush(arena, 1);
      ptr[0] = '-';
   }
   if (value == 0) {
      u8* ptr = mem_arenaPush(arena, 1);
      ptr[0] = '0';
      return;
   }
   
   int popBy = 10 - str__ufast_utoa10(value, (char*) mem_arenaPush(arena, 10));
   if (popBy > 0) {
      mem_arenaPopTo(arena, arena->pos - popBy);
   }
}

LOCAL void str_recordChar(Arena* arena, char value) {
   char* ptr = mem_arenaPush(arena, 1);
   ptr[0] = value;
}

API S8 str_joinVargs(Arena* arena, u32 argCount, va_list inValist) {
   va_list valist;
   va_copy(valist, inValist);
   //va_start(valist, argCount);

   S8 strOut;
   str_record(strOut, arena) {
      for (u32 idx = 0; idx < argCount; idx++) {
         str_Value arg = va_arg(valist, str_Value);
         switch (arg.type) {
            case str_argType_custom:   str_recordCustom(arena, &arg.customVal, (S8) {0, 0}); break;
            case str_argType_char:     str_recordChar(arena, arg.charVal); break;
            case str_argType_str:      str_recordStr(arena, arg.strVal); break;
            case str_argType_f32:      str_recordf32_cast(arena, arg.f32Val); break;
            case str_argType_f64:      str_recordF64(arena, arg.f64Val); break;
            case str_argType_u32:      str_recordU32(arena, arg.u32Val, false, (S8){0, 0}); break;
            case str_argType_u64:      str_recordU64(arena, arg.u64Val, false, (S8){0, 0}); break;
            case str_argType_i32:      str_recordU32(arena, absVal(arg.i32Val), arg.i32Val >= 0 ? false : true, (S8){0, 0}); break;
            case str_argType_i64:      str_recordU64(arena, absVal(arg.i64Val), arg.i64Val >= 0 ? false : true, (S8){0, 0}); break;
         }
      }
   }
   va_end(valist);
   return strOut;
}

S8 str_joinRaw(Arena* arena, u32 argCount, ...) {
   va_list valist;
   va_start(valist, argCount);
   return str_joinVargs(arena, argCount, valist);
}

S8 str_fmtRaw(Arena* arena, S8 fmt, u32 argCount, ...) {
   va_list valist;
   va_start(valist, argCount);
   return str_fmtVargs(arena, fmt, argCount, valist);
}

bx str__insertValue(Arena* arena, u32 insertCount, S8 valueFormat, u32 argCount, va_list inValist) {
   va_list valist;
   va_copy(valist, inValist);
   i32 insertIndex = -1;
   i32 fmtIdx = 0;
   if (valueFormat.size == 0 || valueFormat.content[0] == ':') {
      insertIndex = insertCount;
   } else if (valueFormat.size >= 1 && str_isNumberChar(valueFormat.content[0])) {
      u32 number = 0;
      u64 offset = str_parseU32N(valueFormat, &number);
      if (offset != 0) {
         insertIndex = number;
         fmtIdx = offset;
      }
   }
   str_KeyValue arg;
   if (insertIndex != -1) {
      // Find value at index
      if (insertIndex >= argCount) {
         ASSERT(!"Index out of arguments bounds.");
         va_end(valist);
         return false;
      }
      for (u32 idx = 0; idx < argCount; idx++) {
         arg = va_arg(valist, str_KeyValue);
         if (idx == insertIndex) {
            break;
         }
      }
   } else {
      // figure out S8 key and search for it
      u32 count = 0;
      for (; valueFormat.content[count] != ':' && valueFormat.content[count] != '}'; count++);
      S8 key;
      if (count > 1) {
         key.content = valueFormat.content;
         key.size = count;
         fmtIdx = count;
      } else {
         ASSERT(!"Key argument invalid.");
         va_end(valist);
         return false;
      }
      bx keyFound = false;

      for (u32 idx = 0; idx < argCount; idx++) {
         arg = va_arg(valist, str_KeyValue);
         if (str_isEqual(arg.key, key)) {
            keyFound = true;
            break;
         }
      }

      if (!keyFound) {
         ASSERT(!"Key was not found in arguments");
         va_end(valist);
         return false;
      }
   }
   va_end(valist);

   S8 format;
   format.content = NULL;
   format.size = 0;

   if (valueFormat.content[fmtIdx] == ':') {
      u8 * f = valueFormat.content + (fmtIdx + 1);
      u32 size = valueFormat.size - (fmtIdx + 1);
      u32 count = 0;
      for (; f[count] != '}' && count < size; count++);
      if (count > 0) {
         format.content = f;
         format.size = count;
      }
   }

   switch (arg.value.type) {
      case str_argType_custom:   str_recordCustom(arena, &arg.value.customVal, format); break;
      case str_argType_char:     str_recordChar(arena, arg.value.charVal); break;
      case str_argType_str:      str_recordStr(arena, arg.value.strVal); break;
      case str_argType_f32:      str_recordf32_cast(arena, arg.value.f32Val); break;
      case str_argType_f64:      str_recordF64(arena, arg.value.f64Val); break;
      case str_argType_u32:      str_recordU32(arena, arg.value.u32Val, false, format); break;
      case str_argType_u64:      str_recordU64(arena, arg.value.u64Val, false, format); break;
      case str_argType_i32:      str_recordU32(arena, absVal(arg.value.i32Val), arg.value.i32Val >= 0 ? false : true, format); break;
      case str_argType_i64:      str_recordU64(arena, absVal(arg.value.i64Val), arg.value.i64Val >= 0 ? false : true, format); break;
   }

   return true;
}

S8 str_fmtVargs(Arena* arena, S8 fmt, u32 argCount, va_list list) {
   S8 strOut;
   u32 storedAligmnet = arena->alignment;
   arena->alignment = 1;
   str_record(strOut, arena) {
      char lastChar = 0;
      char currentChar = 0;
      i32 insertCount = 0;
      i32 insertStartIdx = -1;
      i32 textStartIdx = 0;

      for (i32 idx = 0; idx < fmt.size; (lastChar = fmt.content[idx], idx++)) {
         currentChar = fmt.content[idx];
         if (insertStartIdx != -1) {
            if (currentChar == '}' && lastChar != '\\') {
               S8 fmtStr;
               fmtStr.content = fmt.content + insertStartIdx + 1;
               fmtStr.size = idx - (insertStartIdx + 1);
               if (str__insertValue(arena, insertCount, fmtStr, argCount, list)) {
                  textStartIdx = idx + 1;
               } else {
                  textStartIdx = insertStartIdx;
               }
               insertStartIdx = -1;
               insertCount += 1;
            }
         } else if (currentChar == '{') {
            if (!(idx < fmt.size)) {
               ASSERT(!"Encountered single '{' at the end of the string!");
               arena->alignment = storedAligmnet;
               return str_lit("");
            }
            char nextChar = fmt.content[idx + 1];
            if (nextChar == '{') {
               idx += 1;
            }
            i32 textSize = idx - textStartIdx;
            if (textSize > 0) {
               // copy text from template
               u8* mem = (u8*) mem_arenaPush(arena, textSize * sizeof(u8));
               mem_copy(mem, (fmt.content + textStartIdx), textSize * sizeof(u8));
            }
            if (nextChar != '{') {
               insertStartIdx = idx;
            }
            textStartIdx = idx + 1;
         } else if (currentChar == '}') {
            if (!(idx < fmt.size)) {
               ASSERT(!"Encountered single '}' at the end of the string!");
               arena->alignment = storedAligmnet;
               return str_lit("");
            }
            char nextChar = fmt.content[idx + 1];
            if (nextChar == '}') {
               idx += 1;
            } else {
               ASSERT(!"Encountered single '}' ");
               arena->alignment = storedAligmnet;
               return str_lit("");
            }
            i32 textSize = idx - textStartIdx;
            if (textSize > 0) {
               // copy text from template
               u8* mem = (u8*) mem_arenaPush(arena, textSize * sizeof(u8));
               mem_copy(mem, (fmt.content + textStartIdx), textSize * sizeof(u8));
            }
            textStartIdx = idx + 1;
         }
      }
      // copy over the remaining text
      u32 size =  fmt.size - textStartIdx;
      if (size > 0) {
         // copy text from template
         u8* mem = (u8*) mem_arenaPush(arena, size * sizeof(u8));
         mem_copy(mem, (fmt.content + textStartIdx), size * sizeof(u8));
      }
   }

   arena->alignment = storedAligmnet;
   return strOut;
}


str_Builder str_builderInit(Arena* arena, u64 blockDefaultSize) {
   str_Builder builder = {
      .arena = arena,
      .blockDefaultSize = blockDefaultSize ? blockDefaultSize : MEGABYTE(1),
   };

   return builder;
}

S8 str_builderFinish(Arena* targetArena) {
   
}

void str_builderJoinRaw(str_Builder* builder, u32 argCount, ...) {
   ASSERT(builder && "Builder is NULL");
   u32 storedAlignment = builder->arena->alignment;
   str__BuilderBlock* builderBlock = NULL;
   if (builder->lastBlock != NULL && builder->arena->commitPos == builder->arenaLastOffset) {
   } else {
      builderBlock = (str__BuilderBlock*) mem_arenaPush(builder->arena, sizeOf(*builder->lastBlock));
      if (builder->firstBlock == NULL) {
         builder->firstBlock = builderBlock;
         builder->lastBlock = builderBlock;
      } else {
         builder->lastBlock->next = builderBlock;
         builderBlock->prev = builder->lastBlock;
         builder->lastBlock = builderBlock;
      }
   }

   va_list valist;
   va_start(valist, argCount);
   str_joinVargs(builder->arena, argCount, valist);
   builder->arenaLastOffset = builder->arena->commitPos;
   builder->arena->alignment = storedAlignment;
}
void str_builderFmtRaw(str_Builder* builder, S8 fmt, u32 argCount, ...) {
   ASSERT(builder && "Builder is NULL");
   u32 storedAlignment = builder->arena->alignment;
   str__BuilderBlock* builderBlock = NULL;
   if (builder->lastBlock != NULL && builder->arena->commitPos == builder->arenaLastOffset) {
   } else {
      builderBlock = (str__BuilderBlock*) mem_arenaPush(builder->arena, sizeOf(*builder->lastBlock));
      if (builder->firstBlock == NULL) {
         builder->firstBlock = builderBlock;
         builder->lastBlock = builderBlock;
      } else {
         builder->lastBlock->next = builderBlock;
         builderBlock->prev = builder->lastBlock;
         builder->lastBlock = builderBlock;
      }
   }

   va_list valist;
   va_start(valist, argCount);
   str_fmtVargs(builder->arena, fmt, argCount, valist);
   builder->arenaLastOffset = builder->arena->commitPos;
   builder->arena->alignment = storedAlignment;
}


/////////////////
// Parse u32 to str

i32 str__ufast_utoa10(u32 value, char* str) {
   #define JOIN(N)                                                      \
   N "0", N "1", N "2", N "3", N "4", N "5", N "6", N "7", N "8", N "9" \

   #define JOIN2(N)                                                     \
   JOIN(N "0"), JOIN(N "1"), JOIN(N "2"), JOIN(N "3"), JOIN(N "4"),     \
   JOIN(N "5"), JOIN(N "6"), JOIN(N "7"), JOIN(N "8"), JOIN(N "9")      \

   #define JOIN3(N)                                                      \
   JOIN2(N "0"), JOIN2(N "1"), JOIN2(N "2"), JOIN2(N "3"), JOIN2(N "4"), \
   JOIN2(N "5"), JOIN2(N "6"), JOIN2(N "7"), JOIN2(N "8"), JOIN2(N "9")  \

   #define JOIN4                                               \
   JOIN3("0"), JOIN3("1"), JOIN3("2"), JOIN3("3"), JOIN3("4"), \
   JOIN3("5"), JOIN3("6"), JOIN3("7"), JOIN3("8"), JOIN3("9")  \

   #define JOIN5(N)                                                \
   JOIN(N), JOIN(N "1"), JOIN(N "2"), JOIN(N "3"), JOIN(N "4"),    \
   JOIN(N "5"), JOIN(N "6"), JOIN(N "7"), JOIN(N "8"), JOIN(N "9") \

   #define JOIN6                                              \
   JOIN5(""), JOIN2("1"), JOIN2("2"), JOIN2("3"), JOIN2("4"), \
   JOIN2("5"), JOIN2("6"), JOIN2("7"), JOIN2("8"), JOIN2("9") \

   #define F(N)  ((N) >= 100 ? 3 : (N) >= 10 ? 2 : 1)

   #define F10(N)                                   \
   F(N), F(N + 1), F(N + 2), F(N + 3), F(N + 4),    \
   F(N + 5), F(N + 6), F(N + 7), F(N + 8), F(N + 9) \

   #define F100(N)                                    \
   F10(N), F10(N + 10), F10(N + 20), F10(N + 30),     \
   F10(N + 40), F10(N + 50), F10(N + 60), F10(N + 70),\
   F10(N + 80), F10(N + 90)                           \

   static const short offsets[] = {
                                    F100(  0), F100(100), F100(200), F100(300), F100(400),
                                    F100(500), F100(600), F100(700), F100(800), F100(900)
                                    };

   static const char table1[][4] = { JOIN ("") };
   static const char table2[][4] = { JOIN2("") };
   static const char table3[][4] = { JOIN3("") };
   static const char table4[][8] = { JOIN4 };
   static const char table5[][4] = { JOIN6 };

   #undef JOIN
   #undef JOIN2
   #undef JOIN3
   #undef JOIN4
   #undef F
   #undef F10
   #undef F100

   char *wstr;
   #if (_WIN64 || __x86_64__ || __ppc64__)
      uint64_t remains[2];
   #else
      uint32_t remains[2];
   #endif
   unsigned int v2;

   if (value >= 100000000)
   {
      #if (_WIN64 || __x86_64__ || __ppc64__)
         remains[0] = (((uint64_t)value * (uint64_t)3518437209) >> 45);
         remains[1] = (((uint64_t)value * (uint64_t)2882303762) >> 58);
      #else
         remains[0] = value / 10000;
         remains[1] = value / 100000000;
      #endif
      v2 = remains[1];
      remains[1] = remains[0] - remains[1] * 10000;
      remains[0] = value - remains[0] * 10000;
      if (v2 >= 10)
      {
         memcpy(str,table5[v2],2);
         str += 2;
         memcpy(str,table4[remains[1]],4);
         str += 4;
         memcpy(str,table4[remains[0]],4);
         return 10;
      }
      else
      {
         *(char *) str = v2 + '0';
         str += 1;
         memcpy(str,table4[remains[1]],4);
         str += 4;
         memcpy(str,table4[remains[0]],4);
         return 9;
      }
   }
   else if (value >= 10000)
   {
      #if (_WIN64 || __x86_64__ || __ppc64__)
         v2 = (((uint64_t)value * (uint64_t)3518437209 ) >> 45);
      #else
         v2 = value / 10000;
      #endif
      remains[0] = value - v2 * 10000;
      if (v2 >= 1000)
      {
         memcpy(str,table4[v2],4);
         str += 4;
         memcpy(str,table4[remains[0]],4);
         return 8;
      }
      else
      {
         wstr = str;
         memcpy(wstr,table5[v2],4);
         wstr += offsets[v2];
         memcpy(wstr,table4[remains[0]],4);
         wstr += 4;
         return (wstr - str);
      }
   }
   else
   {
      if (value >= 1000)
      {
         memcpy(str,table4[value],4);
         return 4;
      }
      else if (value >= 100)
      {
         memcpy(str,table3[value],3);
         return 3;
      }
      else if (value >= 10)
      {
         memcpy(str,table2[value],2);
         return 2;
      }
      else
      {
         *(char *) str = *(char *) table1[value];
         return 1;
      }
   }
}

LOCAL i32 str__ufast_itoa10(i32 value, char* str) {
  if (value < 0) { *(str++) = '-'; 
    return str__ufast_utoa10(-value, str) + 1; 
  }
  else return str__ufast_utoa10(value, str);
}

/////////////////
// Parse float in str
// this could be an alternative: https://github.com/charlesnicholson/nanoprintf/blob/bb60443d4821b86482e3c70329fc10280f1e6aa4/nanoprintf.h#L602

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STR__COPYFP(dest, src)                     \
   {                                               \
      int cn;                                      \
      for (cn = 0; cn < 8; cn++)                   \
         ((char *)&dest)[cn] = ((char *)&src)[cn]; \
   }

// get float info
static int32_t str__real_to_parts(int64_t *bits, int32_t *expo, double value)
{
   double d;
   int64_t b = 0;

   // load value and round at the frac_digits
   d = value;

   STR__COPYFP(b, d);

   *bits = b & ((((uint64_t)1) << 52) - 1);
   *expo = (int32_t)(((b >> 52) & 2047) - 1023);

   return (int32_t)((uint64_t) b >> 63);
}

#define STR__SPECIAL 0x7000

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
static uint64_t const str__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000,
   100000000000,
   1000000000000,
   10000000000000,
   100000000000000,
   1000000000000000,
   10000000000000000,
   100000000000000000,
   1000000000000000000,
   10000000000000000000U
};
#define str__tento19th ((uint64_t)1000000000000000000)
#else
static uint64_t const str__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000ULL,
   100000000000ULL,
   1000000000000ULL,
   10000000000000ULL,
   100000000000000ULL,
   1000000000000000ULL,
   10000000000000000ULL,
   100000000000000000ULL,
   1000000000000000000ULL,
   10000000000000000000ULL
};
#define str__tento19th (1000000000000000000ULL)
#endif

#define str__ddmulthi(oh, ol, xh, yh)                            \
   {                                                               \
      double ahi = 0, alo, bhi = 0, blo;                           \
      int64_t bt;                                             \
      oh = xh * yh;                                                \
      STR__COPYFP(bt, xh);                                       \
      bt &= ((~(uint64_t)0) << 27);                           \
      STR__COPYFP(ahi, bt);                                      \
      alo = xh - ahi;                                              \
      STR__COPYFP(bt, yh);                                       \
      bt &= ((~(uint64_t)0) << 27);                           \
      STR__COPYFP(bhi, bt);                                      \
      blo = yh - bhi;                                              \
      ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo; \
   }

#define str__ddtoS64(ob, xh, xl)          \
   {                                        \
      double ahi = 0, alo, vh, t;           \
      ob = (int64_t)ph;                \
      vh = (double)ob;                      \
      ahi = (xh - vh);                      \
      t = (ahi - xh);                       \
      alo = (xh - (ahi - t)) - (vh + t);    \
      ob += (int64_t)(ahi + alo + xl); \
   }

#define str__ddrenorm(oh, ol) \
   {                            \
      double s;                 \
      s = oh + ol;              \
      ol = ol - (s - oh);       \
      oh = s;                   \
   }

#define str__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define str__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void str__raiseToPower10(double *ohi, double *olo, double d, int32_t power) { // power can be -323 to +350
    static double const str__bot[23] = {
        1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
        1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
    };
    static double const str__negbot[22] = {
        1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
        1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
    };
    static double const str__negboterr[22] = {
        -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
        4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
        -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
        2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
    };
    static double const str__top[13] = {
        1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
    };
    static double const str__negtop[13] = {
        1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
    };
    static double const str__toperr[13] = {
        8388608,
        6.8601809640529717e+028,
        -7.253143638152921e+052,
        -4.3377296974619174e+075,
        -1.5559416129466825e+098,
        -3.2841562489204913e+121,
        -3.7745893248228135e+144,
        -1.7356668416969134e+167,
        -3.8893577551088374e+190,
        -9.9566444326005119e+213,
        6.3641293062232429e+236,
        -5.2069140800249813e+259,
        -5.2504760255204387e+282
    };
    static double const str__negtoperr[13] = {
        3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
        -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
        7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
        8.0970921678014997e-317
    };

   double ph, pl;
   if ((power >= 0) && (power <= 22)) {
      str__ddmulthi(ph, pl, d, str__bot[power]);
   } else {
      int32_t e, et, eb;
      double p2h, p2l;

      e = power;
      if (power < 0)
         e = -e;
      et = (e * 0x2c9) >> 14; /* %23 */
      if (et > 13)
         et = 13;
      eb = e - (et * 23);

      ph = d;
      pl = 0.0;
      if (power < 0) {
         if (eb) {
            --eb;
            str__ddmulthi(ph, pl, d, str__negbot[eb]);
            str__ddmultlos(ph, pl, d, str__negboterr[eb]);
         }
         if (et) {
            str__ddrenorm(ph, pl);
            --et;
            str__ddmulthi(p2h, p2l, ph, str__negtop[et]);
            str__ddmultlo(p2h, p2l, ph, pl, str__negtop[et], str__negtoperr[et]);
            ph = p2h;
            pl = p2l;
         }
      } else {
         if (eb) {
            e = eb;
            if (eb > 22)
               eb = 22;
            e -= eb;
            str__ddmulthi(ph, pl, d, str__bot[eb]);
            if (e) {
               str__ddrenorm(ph, pl);
               str__ddmulthi(p2h, p2l, ph, str__bot[e]);
               str__ddmultlos(p2h, p2l, str__bot[e], pl);
               ph = p2h;
               pl = p2l;
            }
         }
         if (et) {
            str__ddrenorm(ph, pl);
            --et;
            str__ddmulthi(p2h, p2l, ph, str__top[et]);
            str__ddmultlo(p2h, p2l, ph, pl, str__top[et], str__toperr[et]);
            ph = p2h;
            pl = p2l;
         }
      }
   }
   str__ddrenorm(ph, pl);
   *ohi = ph;
   *olo = pl;
}

LOCAL i32 str__f32ToCharArr(char const** start, u32 *len, char* out, i32* decimal_pos, f64 value, i32 frac_digits) {
   f64 d;
   i64 bits = 0;
   i32 expo, e, ng, tens;

   d = value;
   STR__COPYFP(bits, d);
   expo = (int32_t)((bits >> 52) & 2047);
   ng = (int32_t)((uint64_t) bits >> 63);
   if (ng)
      d = -d;

   if (expo == 2047) // is nan or inf?
   {
      *start = (bits & ((((uint64_t)1) << 52) - 1)) ? "NaN" : "Inf";
      *decimal_pos = STR__SPECIAL;
      *len = 3;
      return ng;
   }

   if (expo == 0) // is zero or denormal
   {
      if (((uint64_t) bits << 1) == 0) // do zero
      {
         *decimal_pos = 1;
         *start = out;
         out[0] = '0';
         *len = 1;
         return ng;
      }
      // find the right expo for denormals
      {
         int64_t v = ((uint64_t)1) << 51;
         while ((bits & v) == 0) {
            --expo;
            v >>= 1;
         }
      }
   }

   // find the decimal exponent as well as the decimal bits of the value
   {
      double ph, pl;

      // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
      tens = expo - 1023;
      tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

      // move the significant bits into position and stick them into an int
      str__raiseToPower10(&ph, &pl, d, 18 - tens);

      // get full as much precision from double-double as possible
      str__ddtoS64(bits, ph, pl);

      // check if we undershot
      if (((uint64_t)bits) >= str__tento19th)
         ++tens;
   }

   // now do the rounding in integer land
   frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
   if ((frac_digits < 24)) {
      uint32_t dg = 1;
      if ((uint64_t)bits >= str__powten[9])
         dg = 10;
      while ((uint64_t)bits >= str__powten[dg]) {
         ++dg;
         if (dg == 20)
            goto noround;
      }
      if (frac_digits < dg) {
         uint64_t r;
         // add 0.5 at the right position and round
         e = dg - frac_digits;
         if ((uint32_t)e >= 24)
            goto noround;
         r = str__powten[e];
         bits = bits + (r / 2);
         if ((uint64_t)bits >= str__powten[dg])
            ++tens;
         bits /= r;
      }
   noround:;
   }

   // kill long trailing runs of zeros
   if (bits) {
      uint32_t n;
      for (;;) {
         if (bits <= 0xffffffff)
            break;
         if (bits % 1000)
            goto donez;
         bits /= 1000;
      }
      n = (uint32_t)bits;
      while ((n % 1000) == 0)
         n /= 1000;
      bits = n;
   donez:;
   }
   static const struct {
     short temp; // force next field to be 2-byte aligned
     char pair[201];
   } str__digitpair = {
     0,
     "00010203040506070809101112131415161718192021222324"
     "25262728293031323334353637383940414243444546474849"
     "50515253545556575859606162636465666768697071727374"
     "75767778798081828384858687888990919293949596979899"
   };

   // convert to string
   out += 64;
   e = 0;
   for (;;) {
      uint32_t n;
      char *o = out - 8;
      // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
      if (bits >= 100000000) {
         n = (uint32_t)(bits % 100000000);
         bits /= 100000000;
      } else {
         n = (uint32_t)bits;
         bits = 0;
      }
      while (n) {
         out -= 2;
         *(uint16_t *)out = *(uint16_t *)&str__digitpair.pair[(n % 100) * 2];
         n /= 100;
         e += 2;
      }
      if (bits == 0) {
         if ((e) && (out[0] == '0')) {
            ++out;
            --e;
         }
         break;
      }
      while (out != o) {
         *--out = '0';
         ++e;
      }
   }

   *decimal_pos = tens;
   *start = out;
   *len = e;
   return ng;
}

S8 str_floatToStr(f64 value, S8 storeStr, i32* decimalPos, i32 fracDigits) {
   fracDigits = fracDigits >= 0 ? fracDigits : 15;
   fracDigits = minVal(fracDigits, 15);
   const char* startChar = NULL;
   u8 str[15 * 2];
   u32 length = 0;
   i32 decPos = 0;
   // (char const** start, u32 *len, char* out, i32* decimal_pos, f64 value, i32 frac_digits)
   i32 start = str__f32ToCharArr(&startChar, &length, (char*) storeStr.content, decimalPos, value, fracDigits);
   S8 outStr;
   outStr.content = length == 0 ? NULL : storeStr.content + start;
   outStr.size = length;
   return outStr;
}

S8 str_u32ToHex(Arena* arena, u32 value) {
   static const u8 digits[] = "0123456789ABCDEF";
   static const u32 hexLen = 4 << 1;
   u8 strTmp[4 << 1];
   strTmp[0] = digits[0];
   u32 i = 0;
   for (u32 j = (hexLen - 1) * 4, f = 0; f < hexLen;j -= 4, f += 1) {
      u32 digitIdx = value >> j;
      if (i == 0 && digitIdx == 0) {
         continue;
      }
      strTmp[i] = digits[(value >> j) & 0x0f];
      i += 1;
   }
   S8 raw;
   raw.content = &strTmp[0];
   raw.size = i == 0 ? 1 : i;

   return str_join(arena, s8("0x"), raw);
}
