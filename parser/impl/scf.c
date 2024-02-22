#include "base/base.h"
#include "base/base_types.h"
#include "base/base_str.h"
#include "parser/tokenizer.h"
#include "parser/scf.h"

bool scf_next(scf_Global* scf) {
    u64 lineStart = 0;
    u64 needleStart = scf->needle;
    if (str_isEndOfLineChar(scf->str.content[scf->needle])) {
        scf->line += 1;
    }
    for (bool running = true; running;) {
        char c = scf->str.content[scf->needle++];
        switch (c) {
            case '[': {
                u64 catNameStart = 0;
                u64 catNameEnd = 0;
                for (;scf->needle < scf->str.size && c != '\n'; scf->needle++) {
                    c = scf->str.content[scf->needle];
                    if (str_isSpacingChar(c)) {
                        if (catNameStart == 0) {
                            continue;
                        }
                        catNameEnd = scf->needle;
                        break;
                    }
                    if (c == ']') {
                        if (catNameStart != 0) {
                            catNameEnd = scf->needle;
                        }
                        scf->needle++;
                        break;
                    }
                    if (catNameStart == 0) {
                        catNameStart = scf->needle;
                    }
                }

                if (catNameStart == 0) {
                    // error
                }

                for (;scf->needle < scf->str.size && c != '\n'; scf->needle++) {
                    c = scf->str.content[scf->needle];
                    if (str_isSpacingChar(c)) {
                        continue;
                    }
                }

                scf->category = str_subStr(scf->str, catNameStart, catNameEnd - catNameStart);
                // skip the rest of the line
                for (;scf->needle < scf->str.size && c != '\n' && c != '#'; scf->needle++) {
                    c = scf->str.content[scf->needle];
                    if (str_isSpacingChar(c)) {
                        continue;
                    }
                    // error nothing allowed but comments/whitespace on the same line as an option
                }
                scf->line += 1;
                scf->valueType = scf_type_category;
                return true;
            } break;
            case '#': {
                // skip line
                for (;scf->needle < scf->str.size && c != '\n'; scf->needle++) {
                    c = scf->str.content[scf->needle];
                    if (c == '\n') {
                        break;
                    }
                }
            } break;
            case '\n': {
                scf->line += 1;
                lineStart = scf->needle;
            } break;
            default: {
                if (str_isSpacingChar(c)) {
                    break;
                }
                
                for (;scf->needle < scf->str.size && str_isSpacingChar(scf->str.content[scf->needle]); scf->needle++);
                if (scf->needle >= scf->str.size) {
                    return false;
                }
                if (str_isEndOfLineChar(scf->str.content[scf->needle])) {
                    scf->needle++;
                    scf->line += 1;
                    continue;
                }

                u32 keyNameStart = scf->needle - 1;
                
                // parse key
                c = scf->str.content[scf->needle];
                for (;scf->needle < scf->str.size && !str_isSpacingChar(scf->str.content[scf->needle]); c = scf->str.content[scf->needle++]);
                if (str_isEndOfLineChar(scf->str.content[scf->needle])) {
                    return false; // error
                }
                u32 keyLength = scf->needle - keyNameStart;
                scf->key = str_subStr(scf->str, keyNameStart, keyLength);

                c = scf->str.content[scf->needle];
                for (;scf->needle < scf->str.size && str_isSpacingChar(c); c = scf->str.content[scf->needle++]) {
                    if (str_isEndOfLineChar(c)) {
                        break;
                    }
                    if (c == '=') {
                        break;
                    }
                }

                if (c != '=') {
                    scf->needle = needleStart;
                    return false; // error
                }

                // parse value
                scf->needle += 1; // jump past the "="

                if (scf->needle >= scf->str.size) {
                    scf->needle = needleStart;
                    return false; // error
                }

                c = scf->str.content[scf->needle];
                for (;scf->needle < scf->str.size && str_isSpacingChar(c); c = scf->str.content[scf->needle++]) {
                    if (scf->needle < scf->str.size) {
                        scf->needle = needleStart;
                        return false;
                    }
                }

                
                c = scf->str.content[scf->needle];
                scf_Type type = scf_type_num;
                u32 valueStart = scf->needle;
                u32 lastNonSpaceCharIdx = scf->needle;
                for (;scf->needle < scf->str.size && !str_isEndOfLineChar(c); c = scf->str.content[scf->needle++]) {
                    if (!str_isNumberChar(c)) {
                        type = scf_type_str;
                    }
                    if (!str_isSpacingChar(c)) {
                        lastNonSpaceCharIdx = scf->needle;
                    }
                }
                u32 valueLength = lastNonSpaceCharIdx - valueStart;
                scf->valueStr  = str_subStr(scf->str, valueStart, valueLength);

                if (type == scf_type_str) {
                    if (str_isEqual(scf->valueStr, str8("true"))) {
                        scf->valueType = scf_type_true;
                    } else if (str_isEqual(scf->valueStr, str8("false"))) {
                        scf->valueType = scf_type_false;
                    } else {
                        scf->valueType = type;
                    }
                } else {
                    scf->valueType = type;
                }
                scf->line += 1;
                return true;
            } break;
        }
    }
    return false;
}

scf_Global scf_parse(S8 scfStr, S8 fileName) {
    scf_Global scf = {0};
    scf.fileName = fileName;
    scf.str = scfStr;
    scf.key = str_lit("");
    scf.category = str_lit("");
    return scf;
}

