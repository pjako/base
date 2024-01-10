#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "parser/tokenizer.h"


Str8 tn_getTokenTypeName(tn_tokenType Type) {
    switch(Type) {
        case tn_tokenType_openParen: {return(str_lit("open parentheses"));}
        case tn_tokenType_closeParen: {return(str_lit("close parentheses"));}
        case tn_tokenType_colon: {return(str_lit("colon"));}
        case tn_tokenType_semicolon: {return(str_lit("semicolon"));}
        case tn_tokenType_asterisk: {return(str_lit("asterisk"));}
        case tn_tokenType_openBracket: {return(str_lit("open bracket"));}
        case tn_tokenType_closeBracket: {return(str_lit("close bracket"));}
        case tn_tokenType_openBrace: {return(str_lit("open brace"));}
        case tn_tokenType_closeBrace: {return(str_lit("close brace"));}
        case tn_tokenType_equals: {return(str_lit("equals"));}
        case tn_tokenType_comma: {return(str_lit("comma"));}
        case tn_tokenType_or: {return(str_lit("or"));}
        case tn_tokenType_pound: {return(str_lit("pound"));}
        case tn_tokenType_string: {return(str_lit("string"));}
        case tn_tokenType_identifier: {return(str_lit("identifier"));}
        case tn_tokenType_number: {return(str_lit("number"));}
        case tn_tokenType_spacing: {return(str_lit("whitespace"));}
        case tn_tokenType_endOfLine: {return(str_lit("end of line"));}
        case tn_tokenType_comment: {return(str_lit("comment"));}
        case tn_tokenType_endOfStream: {return(str_lit("end of stream"));}
    }
    
    return(str_lit("unknown"));
};

bool tn_parsing(tn_Tokenizer *tokenizer) {
    bool result = (!tokenizer->error) && tokenizer->input.size > 0;
    return(result);
}

void tn_errorArgList(tn_Tokenizer *tokenizer, tn_Token onToken, char *format, va_list ArgList) {
    // TODO(casey): In the future, if we want to, we can change stream_chunk to
    // take strings and then we could pass the onToken filename/line here.
#if 0
    Outf(tokenizer->ErrorStream, "\\#f00%S(%u,%u)\\#fff: \"%S\" - ", onToken.FileName, onToken.LineNumber, onToken.ColumnNumber, onToken.Text);
    OutfArgList(DEBUG_MEMORY_NAME("ErrorArgList") tokenizer->ErrorStream, format, ArgList);
    Outf(tokenizer->ErrorStream, "\n");
#endif
    ASSERT(!"Error... also implement this properly...");
    tokenizer->error = true;
}

void tn_errorOnToken(tn_Tokenizer *tokenizer, tn_Token onToken, char *format, ...) {
    va_list ArgList;
    va_start(ArgList, format);
#if 0
    ErrorArgList(tokenizer, onToken, format, ArgList);
#endif
    assert(!"Error... also implement this properly...");
    
    va_end(ArgList);
}

void tn_error(tn_Tokenizer *tokenizer, char *format, ...) {
    va_list ArgList;
    va_start(ArgList, format);
    
    tn_Token onToken = tn_peekTokenRaw(tokenizer);
    tn_errorOnToken(tokenizer, onToken, format, ArgList);
    
    va_end(ArgList);
}

void tn_refill(tn_Tokenizer *tokenizer) {
    if(tokenizer->input.size == 0)
    {
        tokenizer->at[0] = 0;
        tokenizer->at[1] = 0;
    }
    else if(tokenizer->input.size == 1)
    {
        tokenizer->at[0] = tokenizer->input.content[0];
        tokenizer->at[1] = 0;
    }
    else
    {
        tokenizer->at[0] = tokenizer->input.content[0];
        tokenizer->at[1] = tokenizer->input.content[1];
    }
}

static char* tn_advance(Str8 *Buffer, uint64_t Count) {
    char *Result = 0;
    
    if (Buffer->size >= Count) {
        Result = (char *) Buffer->content;
        Buffer->content += Count;
        Buffer->size -= Count;
    } else {
        Buffer->content += Buffer->size;
        Buffer->size = 0;
    }
    
    return(Result);
}

void tn_advanceChars(tn_Tokenizer *tokenizer, uint32_t Count) {
    tokenizer->columnNumber += Count;
    tn_advance(&tokenizer->input, Count);
    tn_refill(tokenizer);
}

bool tn_stringsAreEqual(Str8 str, char *B) {
    char* A = (char*) str.content;
    bool result = (A == B);
    
    if (A && B) {
        while(*A && *B && (*A == *B)) {
            ++A;
            ++B;
        }
        
        result = ((*A == 0) && (*B == 0));
    }
    
    return result;
}

bool tn_tokenEquals(tn_Token token, char *Match) {
    bool result = tn_stringsAreEqual(token.text, Match);
    return(result);
}

bool tn_isValid(tn_Token token) {
    bool result = (token.type != tn_tokenType_unknown);
    return(result);
}

static bool tn_isSpacing(char C) {
    return ((C == ' ') || (C == '\t') || (C == '\v') || (C == '\f'));
}

static bool tn_isNumber(char C) {
    return ((C >= '0') && (C <= '9'));
}

static bool tn_isEndOfLine(char C) {
    return ((C == '\n') || (C == '\r'));
}

static bool tn_isAlpha(char C) {
    return (((C >= 'a') && (C <= 'z')) || ((C >= 'A') && (C <= 'Z')));
}

static i32 tn_roundReal32ToInt32(float real32) {
    //i32 Result = _mm_cvtss_si32(_mm_set_ss(Real32));
    // TODO: this could overflow, fix!
    return (int) real32;
}

tn_Token tn_getTokenRaw(tn_Tokenizer *tokenizer) {
    tn_Token token = {0};
    token.fileName = tokenizer->fileName;
    token.columnNumber = tokenizer->columnNumber;
    token.lineNumber = tokenizer->lineNumber;
    token.text = tokenizer->input;
    
    char C = tokenizer->at[0];
    tn_advanceChars(tokenizer, 1);
    switch(C)
    {
        case '\0': {token.type = tn_tokenType_endOfStream;} break;
        
        case '(': {token.type = tn_tokenType_openParen;} break;
        case ')': {token.type = tn_tokenType_closeParen;} break;
        case ':': {token.type = tn_tokenType_colon;} break;
        case ';': {token.type = tn_tokenType_semicolon;} break;
        case '*': {token.type = tn_tokenType_asterisk;} break;
        case '[': {token.type = tn_tokenType_openBracket;} break;
        case ']': {token.type = tn_tokenType_closeBracket;} break;
        case '{': {token.type = tn_tokenType_openBrace;} break;
        case '}': {token.type = tn_tokenType_closeBrace;} break;
        case '=': {token.type = tn_tokenType_equals;} break;
        case ',': {token.type = tn_tokenType_comma;} break;
        case '|': {token.type = tn_tokenType_or;} break;
        case '#': {token.type = tn_tokenType_pound;} break;
        
        case '"':
        {
            token.type = tn_tokenType_string;
            
            while(tokenizer->at[0] && tokenizer->at[0] != '"') {
                if((tokenizer->at[0] == '\\') && tokenizer->at[1]) {
                    tn_advanceChars(tokenizer, 1);
                }
                tn_advanceChars(tokenizer, 1);
            }
            
            if(tokenizer->at[0] == '"') {
                tn_advanceChars(tokenizer, 1);
            }
        } break;
        
        default:
        {
            if(tn_isSpacing(C)) {
                token.type = tn_tokenType_spacing;
                while(tn_isSpacing(tokenizer->at[0])) {
                    tn_advanceChars(tokenizer, 1);
                }
            } else if(tn_isEndOfLine(C)) {
                token.type = tn_tokenType_endOfLine;
                if (((C == '\r') && (tokenizer->at[0] == '\n')) || ((C == '\n') && (tokenizer->at[0] == '\r'))) {
                    tn_advanceChars(tokenizer, 1);
                }
                
                tokenizer->columnNumber = 1;
                ++tokenizer->lineNumber;
            } else if ((C == '/') && (tokenizer->at[0] == '/')) {
                token.type = tn_tokenType_comment;
                
                tn_advanceChars(tokenizer, 2);
                while(tokenizer->at[0] && !tn_isEndOfLine(tokenizer->at[0])) {
                    tn_advanceChars(tokenizer, 1);
                }
            } else if ((C == '/') && (tokenizer->at[0] == '*')) {
                token.type = tn_tokenType_comment;
                
                tn_advanceChars(tokenizer, 2);
                while(tokenizer->at[0] && !((tokenizer->at[0] == '*') && (tokenizer->at[1] == '/'))) {
                    if(((tokenizer->at[0] == '\r') && (tokenizer->at[1] == '\n')) || ((tokenizer->at[0] == '\n') && (tokenizer->at[1] == '\r'))) {
                        tn_advanceChars(tokenizer, 1);
                    }
                    
                    if(tn_isEndOfLine(tokenizer->at[0])) {
                        ++tokenizer->lineNumber;
                    }
                    
                    tn_advanceChars(tokenizer, 1);
                }
                
                if(tokenizer->at[0] == '*') {
                    tn_advanceChars(tokenizer, 2);
                }
            } else if(tn_isAlpha(C)) {
                token.type = tn_tokenType_identifier;
                
                while(tn_isAlpha(tokenizer->at[0]) || tn_isNumber(tokenizer->at[0]) || (tokenizer->at[0] == '_')) {
                    tn_advanceChars(tokenizer, 1);
                }
            }
            else if(tn_isNumber(C)) {
                float Number = (float)(C - '0');
                
                while(tn_isNumber(tokenizer->at[0])) {
                    float Digit = (float)(tokenizer->at[0] - '0');
                    Number = 10.0f * Number + Digit;
                    tn_advanceChars(tokenizer, 1);
                }
                
                if(tokenizer->at[0] == '.') {
                    tn_advanceChars(tokenizer, 1);
                    float Coefficient = 0.1f;
                    while (tn_isNumber(tokenizer->at[0])) {
                        float Digit = (float)(tokenizer->at[0] - '0');
                        Number += Coefficient*Digit;
                        Coefficient *= 0.1f;
                        tn_advanceChars(tokenizer, 1);
                    }
                }
                
                token.type = tn_tokenType_number;
                token.f32 = Number;
                token.i32 = tn_roundReal32ToInt32(Number);
            } else {
                token.type = tn_tokenType_unknown;
            }
        } break;
    }
    
    token.text.size = (tokenizer->input.content - token.text.content);
    
    return(token);
}

tn_Token tn_peekTokenRaw(tn_Tokenizer* tokenizer) {
    tn_Tokenizer temp = *tokenizer;
    tn_Token result = tn_getTokenRaw(tokenizer);
    *tokenizer = temp;
    return(result);
}

tn_Token tn_getToken(tn_Tokenizer* tokenizer) {
    tn_Token token;
    for (;;) {
        token = tn_getTokenRaw(tokenizer);
        if ((token.type == tn_tokenType_spacing) || (token.type == tn_tokenType_endOfLine) || (token.type == tn_tokenType_comment)) {
            // NOTE(casey): Ignore these when we're getting "real" tokens
        } else {
            if(token.type == tn_tokenType_string) {
                if(token.text.size && (token.text.content[0] == '"')) {
                    ++token.text.content;
                    --token.text.size;
                }
                
                if(token.text.size && (token.text.content[token.text.size - 1] == '"')) {
                    --token.text.size;
                }
            }
            
            break;
        }
    }
    
    return(token);
}

tn_Token tn_peekToken(tn_Tokenizer* tokenizer) {
    tn_Tokenizer temp = *tokenizer;
    tn_Token result = tn_getToken(&temp);
    return(result);
}

tn_Token tn_requireToken(tn_Tokenizer* tokenizer, tn_tokenType desiredType) {
    tn_Token token = tn_getToken(tokenizer);
    if (token.type != desiredType) {
        tn_errorOnToken(tokenizer, token, "Unexpected token type (expected %S)", tn_getTokenTypeName(desiredType));
    }
    
    return(token);
}

tn_Token tn_requireIdentifier(tn_Tokenizer* tokenizer, char *Match) {
    tn_Token ID = tn_requireToken(tokenizer, tn_tokenType_identifier);
    if(!tn_tokenEquals(ID, Match)) {
        tn_errorOnToken(tokenizer, ID, "Expected \"%s\"", Match);
    }
    
    return(ID);
}

tn_Token tn_requireIntegerRange(tn_Tokenizer* tokenizer, i32 MinValue, i32 MaxValue) {
    tn_Token token = tn_requireToken(tokenizer, tn_tokenType_number);
    if(token.type == tn_tokenType_number) {
        if((token.i32 >= MinValue) && (token.i32 <= MaxValue)) {
            // NOTE(casey): Valid
        } else {
            tn_errorOnToken(tokenizer, token, "Expected a number between %d and %d", MinValue, MaxValue);
        }
    }
    
    return(token);
}

bool tn_optionalToken(tn_Tokenizer* tokenizer, tn_tokenType desiredType) {
    tn_Token token = tn_peekToken(tokenizer);
    bool result = (token.type == desiredType);
    if (result) {
        tn_getToken(tokenizer);
    }
    
    return(result);
}

tn_Tokenizer tn_createTokenize(Str8 Input, Str8 FileName) {
    tn_Tokenizer result = {0};
    
    result.fileName = FileName;
    result.columnNumber = 1;
    result.lineNumber = 1;
    result.input = Input;
    tn_refill(&result);
    
    return(result);
}
