#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_
#ifdef __cplusplus
extern "C" {
#endif

#define TN_API extern

typedef struct tn_stream tn_stream;

typedef enum tn_tokenType {
    tn_tokenType_unknown,
    tn_tokenType_openParen,
    tn_tokenType_closeParen,
    tn_tokenType_colon,
    tn_tokenType_semicolon,
    tn_tokenType_asterisk,
    tn_tokenType_openBracket,
    tn_tokenType_closeBracket,
    tn_tokenType_openBrace,
    tn_tokenType_closeBrace,
    tn_tokenType_equals,
    tn_tokenType_comma,
    tn_tokenType_or,
    tn_tokenType_pound,
    tn_tokenType_string,
    tn_tokenType_identifier,
    tn_tokenType_number,
    tn_tokenType_spacing,
    tn_tokenType_endOfLine,
    tn_tokenType_comment,
    tn_tokenType_endOfStream,
} tn_tokenType;

typedef struct tn_Token {
    Str8 fileName;
    i32 columnNumber;
    i32 lineNumber;
    
    tn_tokenType type;
    Str8 text;
    float f32;
    i32 i32;
} tn_Token;

typedef struct tn_Tokenizer {
    Str8 fileName;
    i32 columnNumber;
    i32 lineNumber;
    tn_stream *errorStream;
    
    Str8 input;
    char at[2];
    
    bool error;
} tn_Tokenizer;

TN_API bool tn_stringsAreEqual(Str8 str, char *B);
TN_API bool tn_parsing(tn_Tokenizer* tokenizer);
TN_API void tn_errorOnToken(tn_Tokenizer* tokenizer, tn_Token OnToken, char *Format, ...);
TN_API void tn_error(tn_Tokenizer* tokenizer, char *Format, ...);
TN_API bool tn_isValid(tn_Token Token);
TN_API bool tn_tokenEquals(tn_Token Token, char *Match);
TN_API tn_Token tn_getTokenRaw(tn_Tokenizer* tokenizer);
TN_API tn_Token tn_peekTokenRaw(tn_Tokenizer* tokenizer);
TN_API tn_Token tn_getToken(tn_Tokenizer* tokenizer);
TN_API tn_Token tn_peekToken(tn_Tokenizer* tokenizer);
TN_API tn_Token tn_requireToken(tn_Tokenizer* tokenizer, tn_tokenType DesiredType);
TN_API tn_Token tn_requireIdentifier(tn_Tokenizer* tokenizer, char *Match);
TN_API tn_Token tn_requireIntegerRange(tn_Tokenizer* tokenizer, i32 MinValue, i32 MaxValue);
TN_API bool tn_optionalToken(tn_Tokenizer* tokenizer, tn_tokenType DesiredType);
TN_API tn_Tokenizer tn_createTokenize(Str8 Input, Str8 FileName);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _TOKENIZER_H_ */
