

















#ifndef READ_H
#define READ_H 1

#include "unicode/utypes.h"
#include "ustr.h"
#include "ucbuf.h"



enum ETokenType
{
    TOK_STRING,          
    TOK_OPEN_BRACE,      
    TOK_CLOSE_BRACE,     
    TOK_COMMA,           
    TOK_COLON,           

    TOK_EOF,             
    TOK_ERROR,           
    TOK_TOKEN_COUNT      
};

U_CFUNC UChar32 unescape(UCHARBUF *buf, UErrorCode *status);

U_CFUNC void resetLineNumber(void);

U_CFUNC enum ETokenType
getNextToken(UCHARBUF *buf,
             struct UString *token,
             uint32_t *linenumber, 
             struct UString *comment,
             UErrorCode *status);

#endif
