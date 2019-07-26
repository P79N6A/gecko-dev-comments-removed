

















#include "read.h"
#include "errmsg.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"

#define OPENBRACE    0x007B
#define CLOSEBRACE   0x007D
#define COMMA        0x002C
#define QUOTE        0x0022
#define ESCAPE       0x005C
#define SLASH        0x002F
#define ASTERISK     0x002A
#define SPACE        0x0020
#define COLON        0x003A
#define BADBOM       0xFFFE
#define CR           0x000D
#define LF           0x000A
               
static int32_t lineCount;


static enum ETokenType getStringToken(UCHARBUF *buf,
                                      UChar32 initialChar,
                                      struct UString *token,
                                      UErrorCode *status);

static UChar32 getNextChar           (UCHARBUF *buf, UBool skipwhite, struct UString *token, UErrorCode *status);
static void    seekUntilNewline      (UCHARBUF *buf, struct UString *token, UErrorCode *status);
static void    seekUntilEndOfComment (UCHARBUF *buf, struct UString *token, UErrorCode *status);
static UBool   isWhitespace          (UChar32 c);
static UBool   isNewline             (UChar32 c);

U_CFUNC void resetLineNumber() {
    lineCount = 1;
}









U_CFUNC enum ETokenType
getNextToken(UCHARBUF* buf,
             struct UString *token,
             uint32_t *linenumber, 
             struct UString *comment,
             UErrorCode *status) {
    enum ETokenType result;
    UChar32         c;

    if (U_FAILURE(*status)) {
        return TOK_ERROR;
    }

    
    c = getNextChar(buf, TRUE, comment, status);

    if (U_FAILURE(*status)) {
        return TOK_ERROR;
    }

    *linenumber = lineCount;

    switch(c) {
    case BADBOM:
        return TOK_ERROR;
    case OPENBRACE:
        return TOK_OPEN_BRACE;
    case CLOSEBRACE:
        return TOK_CLOSE_BRACE;
    case COMMA:
        return TOK_COMMA;
    case U_EOF:
        return TOK_EOF;
    case COLON:
        return TOK_COLON;

    default:
        result = getStringToken(buf, c, token, status);
    }

    *linenumber = lineCount;
    return result;
}












static enum ETokenType getStringToken(UCHARBUF* buf,
                                      UChar32 initialChar,
                                      struct UString *token,
                                      UErrorCode *status) {
    UBool    lastStringWasQuoted;
    UChar32  c;
    UChar    target[3] = { '\0' };
    UChar    *pTarget   = target;
    int      len=0;
    UBool    isFollowingCharEscaped=FALSE;
    UBool    isNLUnescaped = FALSE;
    UChar32  prevC=0;

    




    if (U_FAILURE(*status)) {
        return TOK_ERROR;
    }

    
    lastStringWasQuoted = FALSE;
    c = initialChar;
    ustr_setlen(token, 0, status);

    if (U_FAILURE(*status)) {
        return TOK_ERROR;
    }

    for (;;) {
        if (c == QUOTE) {
            if (!lastStringWasQuoted && token->fLength > 0) {
                ustr_ucat(token, SPACE, status);

                if (U_FAILURE(*status)) {
                    return TOK_ERROR;
                }
            }

            lastStringWasQuoted = TRUE;

            for (;;) {
                c = ucbuf_getc(buf,status);

                
                if (c == U_EOF) {
                    return TOK_EOF;
                }

                
                if (U_FAILURE(*status)) {
                    return TOK_ERROR;
                }

                if (c == QUOTE && !isFollowingCharEscaped) {
                    break;
                }

                if (c == ESCAPE  && !isFollowingCharEscaped) {
                    pTarget = target;
                    c       = unescape(buf, status);

                    if (c == U_ERR) {
                        return TOK_ERROR;
                    }
                    if(c == CR || c == LF){
                        isNLUnescaped = TRUE;
                    }
                }               

                if(c==ESCAPE && !isFollowingCharEscaped){
                    isFollowingCharEscaped = TRUE;
                }else{
                    U_APPEND_CHAR32(c, pTarget,len);
                    pTarget = target;
                    ustr_uscat(token, pTarget,len, status);
                    isFollowingCharEscaped = FALSE;
                    len=0;
                    if(c == CR || c == LF){
                        if(isNLUnescaped == FALSE && prevC!=CR){
                            lineCount++;
                        }
                        isNLUnescaped = FALSE;
                    }
                }
                
                if (U_FAILURE(*status)) {
                    return TOK_ERROR;
                }
                prevC = c;
            }
        } else {
            if (token->fLength > 0) {
                ustr_ucat(token, SPACE, status);

                if (U_FAILURE(*status)) {
                    return TOK_ERROR;
                }
            }
            
            if(lastStringWasQuoted){
                if(getShowWarning()){
                    warning(lineCount, "Mixing quoted and unquoted strings");
                }
                if(isStrict()){
                    return TOK_ERROR;
                }

            }

            lastStringWasQuoted = FALSE;
            
            





            if (c == ESCAPE) {
                pTarget = target;
                c       = unescape(buf, status);

                
                if (c == U_EOF) {
                    return TOK_ERROR;
                }
            }

            U_APPEND_CHAR32(c, pTarget,len);
            pTarget = target;
            ustr_uscat(token, pTarget,len, status);
            len=0;
            
            if (U_FAILURE(*status)) {
                return TOK_ERROR;
            }

            for (;;) {
                
                c = getNextChar(buf, FALSE, NULL, status);

                
                if (c == U_EOF) {
                    ucbuf_ungetc(c, buf);
                    return TOK_STRING;
                }

                if (U_FAILURE(*status)) {
                    return TOK_STRING;
                }

                if (c == QUOTE
                        || c == OPENBRACE
                        || c == CLOSEBRACE
                        || c == COMMA
                        || c == COLON) {
                    ucbuf_ungetc(c, buf);
                    break;
                }

                if (isWhitespace(c)) {
                    break;
                }

                if (c == ESCAPE) {
                    pTarget = target;
                    c       = unescape(buf, status);

                    if (c == U_ERR) {
                        return TOK_ERROR;
                    }
                }

                U_APPEND_CHAR32(c, pTarget,len);
                pTarget = target;
                ustr_uscat(token, pTarget,len, status);
                len=0;
                if (U_FAILURE(*status)) {
                    return TOK_ERROR;
                }
            }
        }

        
        c = getNextChar(buf, TRUE, NULL, status);

        if (U_FAILURE(*status)) {
            return TOK_STRING;
        }

        if (c == OPENBRACE || c == CLOSEBRACE || c == COMMA || c == COLON) {
            ucbuf_ungetc(c, buf);
            return TOK_STRING;
        }
    }
}



static UChar32 getNextChar(UCHARBUF* buf,
                           UBool skipwhite,
                           struct UString *token,
                           UErrorCode *status) {
    UChar32 c, c2;

    if (U_FAILURE(*status)) {
        return U_EOF;
    }

    for (;;) {
        c = ucbuf_getc(buf,status);

        if (c == U_EOF) {
            return U_EOF;
        }

        if (skipwhite && isWhitespace(c)) {
            continue;
        }

        
        if (c != SLASH) {
            return c;
        }

        c = ucbuf_getc(buf,status); 

        if (c == U_EOF) {
            return U_EOF;
        }

        switch (c) {
        case SLASH:  
            seekUntilNewline(buf, NULL, status);
            break;

        case ASTERISK:  
            c2 = ucbuf_getc(buf, status); 
            if(c2 == ASTERISK){  
                
                seekUntilEndOfComment(buf, token, status);
            } else {
                ucbuf_ungetc(c2, buf); 
                seekUntilEndOfComment(buf, NULL, status);
            }
            break;

        default:
            ucbuf_ungetc(c, buf); 
            
            return SLASH;
        }

    }
}

static void seekUntilNewline(UCHARBUF* buf,
                             struct UString *token,
                             UErrorCode *status) {
    UChar32 c;

    if (U_FAILURE(*status)) {
        return;
    }

    do {
        c = ucbuf_getc(buf,status);
        
        if(token!=NULL){
            ustr_u32cat(token, c, status);
        }
    } while (!isNewline(c) && c != U_EOF && *status == U_ZERO_ERROR);
}

static void seekUntilEndOfComment(UCHARBUF *buf,
                                  struct UString *token,
                                  UErrorCode *status) {
    UChar32  c, d;
    uint32_t line;

    if (U_FAILURE(*status)) {
        return;
    }

    line = lineCount;

    do {
        c = ucbuf_getc(buf, status);

        if (c == ASTERISK) {
            d = ucbuf_getc(buf, status);

            if (d != SLASH) {
                ucbuf_ungetc(d, buf);
            } else {
                break;
            }
        }
        
        if(token!=NULL){
            ustr_u32cat(token, c, status);
        }
        
        isNewline(c);

    } while (c != U_EOF && *status == U_ZERO_ERROR);

    if (c == U_EOF) {
        *status = U_INVALID_FORMAT_ERROR;
        error(line, "unterminated comment detected");
    }
}

U_CFUNC UChar32 unescape(UCHARBUF *buf, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return U_EOF;
    }

    

    ucbuf_ungetc(ESCAPE, buf);

    return ucbuf_getcx32(buf, status);
}

static UBool isWhitespace(UChar32 c) {
    switch (c) {
        
    case 0x000A:
    case 0x2029:
        lineCount++;
    case 0x000D:
    case 0x0020:
    case 0x0009:
    case 0xFEFF:
        return TRUE;

    default:
        return FALSE;
    }
}

static UBool isNewline(UChar32 c) {
    switch (c) {
        
    case 0x000A:
    case 0x2029:
        lineCount++;
    case 0x000D:
        return TRUE;

    default:
        return FALSE;
    }
}
