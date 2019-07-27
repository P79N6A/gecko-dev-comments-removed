



#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "prefread.h"
#include "nsString.h"
#include "nsUTF8Utils.h"

#ifdef TEST_PREFREAD
#include <stdio.h>
#define NS_WARNING(_s) printf(">>> " _s "!\n")
#define NS_NOTREACHED(_s) NS_WARNING(_s)
#else
#include "nsDebug.h" 
#endif


enum {
    PREF_PARSE_INIT,
    PREF_PARSE_MATCH_STRING,
    PREF_PARSE_UNTIL_NAME,
    PREF_PARSE_QUOTED_STRING,
    PREF_PARSE_UNTIL_COMMA,
    PREF_PARSE_UNTIL_VALUE,
    PREF_PARSE_INT_VALUE,
    PREF_PARSE_COMMENT_MAYBE_START,
    PREF_PARSE_COMMENT_BLOCK,
    PREF_PARSE_COMMENT_BLOCK_MAYBE_END,
    PREF_PARSE_ESC_SEQUENCE,
    PREF_PARSE_HEX_ESCAPE,
    PREF_PARSE_UTF16_LOW_SURROGATE,
    PREF_PARSE_UNTIL_OPEN_PAREN,
    PREF_PARSE_UNTIL_CLOSE_PAREN,
    PREF_PARSE_UNTIL_SEMICOLON,
    PREF_PARSE_UNTIL_EOL
};

#define UTF16_ESC_NUM_DIGITS    4
#define HEX_ESC_NUM_DIGITS      2
#define BITS_PER_HEX_DIGIT      4

static const char kUserPref[] = "user_pref";
static const char kPref[] = "pref";
static const char kPrefSticky[] = "sticky_pref";
static const char kTrue[] = "true";
static const char kFalse[] = "false";





















static bool
pref_GrowBuf(PrefParseState *ps)
{
    int bufLen, curPos, valPos;

    bufLen = ps->lbend - ps->lb;
    curPos = ps->lbcur - ps->lb;
    valPos = ps->vb    - ps->lb;

    if (bufLen == 0)
        bufLen = 128;  
    else
        bufLen <<= 1;  

#ifdef TEST_PREFREAD
    fprintf(stderr, ">>> realloc(%d)\n", bufLen);
#endif

    ps->lb = (char*) realloc(ps->lb, bufLen);
    if (!ps->lb)
        return false;

    ps->lbcur = ps->lb + curPos;
    ps->lbend = ps->lb + bufLen;
    ps->vb    = ps->lb + valPos;

    return true;
}












static bool
pref_DoCallback(PrefParseState *ps)
{
    PrefValue  value;

    switch (ps->vtype) {
    case PREF_STRING:
        value.stringVal = ps->vb;
        break;
    case PREF_INT:
        if ((ps->vb[0] == '-' || ps->vb[0] == '+') && ps->vb[1] == '\0') {
            NS_WARNING("malformed integer value");
            return false;
        }
        value.intVal = atoi(ps->vb);
        break;
    case PREF_BOOL:
        value.boolVal = (ps->vb == kTrue);
        break;
    default:
        break;
    }
    (*ps->reader)(ps->closure, ps->lb, value, ps->vtype, ps->fdefault,
                  ps->fstickydefault);
    return true;
}

void
PREF_InitParseState(PrefParseState *ps, PrefReader reader, void *closure)
{
    memset(ps, 0, sizeof(*ps));
    ps->reader = reader;
    ps->closure = closure;
}

void
PREF_FinalizeParseState(PrefParseState *ps)
{
    if (ps->lb)
        free(ps->lb);
}






















bool
PREF_ParseBuf(PrefParseState *ps, const char *buf, int bufLen)
{
    const char *end;
    char c;
    char udigit;
    int state;

    state = ps->state;
    for (end = buf + bufLen; buf != end; ++buf) {
        c = *buf;
        switch (state) {
        
        case PREF_PARSE_INIT:
            if (ps->lbcur != ps->lb) { 
                ps->lbcur = ps->lb;
                ps->vb    = nullptr;
                ps->vtype = PREF_INVALID;
                ps->fdefault = false;
                ps->fstickydefault = false;
            }
            switch (c) {
            case '/':       
                state = PREF_PARSE_COMMENT_MAYBE_START;
                break;
            case '#':       
                state = PREF_PARSE_UNTIL_EOL;
                break;
            case 'u':       
            case 'p':       
            case 's':       
                ps->smatch = (c == 'u' ? kUserPref :
                             (c == 's' ? kPrefSticky : kPref));
                ps->sindex = 1;
                ps->nextstate = PREF_PARSE_UNTIL_OPEN_PAREN;
                state = PREF_PARSE_MATCH_STRING;
                break;
            
            }
            break;

        
        case PREF_PARSE_MATCH_STRING:
            if (c == ps->smatch[ps->sindex++]) {
                
                if (ps->smatch[ps->sindex] == '\0') {
                    state = ps->nextstate;
                    ps->nextstate = PREF_PARSE_INIT; 
                }
                
            }
            else {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;

        
        case PREF_PARSE_QUOTED_STRING:
            
            if (ps->lbcur == ps->lbend && !pref_GrowBuf(ps))
                return false; 
            if (c == '\\')
                state = PREF_PARSE_ESC_SEQUENCE;
            else if (c == ps->quotechar) {
                *ps->lbcur++ = '\0';
                state = ps->nextstate;
                ps->nextstate = PREF_PARSE_INIT; 
            }
            else
                *ps->lbcur++ = c;
            break;

        
        case PREF_PARSE_UNTIL_NAME:
            if (c == '\"' || c == '\'') {
                ps->fdefault = (ps->smatch == kPref || ps->smatch == kPrefSticky);
                ps->fstickydefault = (ps->smatch == kPrefSticky);
                ps->quotechar = c;
                ps->nextstate = PREF_PARSE_UNTIL_COMMA; 
                state = PREF_PARSE_QUOTED_STRING;
            }
            else if (c == '/') {       
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;

        
        case PREF_PARSE_UNTIL_COMMA:
            if (c == ',') {
                ps->vb = ps->lbcur;
                state = PREF_PARSE_UNTIL_VALUE;
            }
            else if (c == '/') {       
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;

        
        case PREF_PARSE_UNTIL_VALUE:
            

            if (c == '\"' || c == '\'') {
                ps->vtype = PREF_STRING;
                ps->quotechar = c;
                ps->nextstate = PREF_PARSE_UNTIL_CLOSE_PAREN;
                state = PREF_PARSE_QUOTED_STRING;
            }
            else if (c == 't' || c == 'f') {
                ps->vb = (char *) (c == 't' ? kTrue : kFalse);
                ps->vtype = PREF_BOOL;
                ps->smatch = ps->vb;
                ps->sindex = 1;
                ps->nextstate = PREF_PARSE_UNTIL_CLOSE_PAREN;
                state = PREF_PARSE_MATCH_STRING;
            }
            else if (isdigit(c) || (c == '-') || (c == '+')) {
                ps->vtype = PREF_INT;
                
                if (ps->lbcur == ps->lbend && !pref_GrowBuf(ps))
                    return false; 
                *ps->lbcur++ = c;
                state = PREF_PARSE_INT_VALUE;
            }
            else if (c == '/') {       
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;
        case PREF_PARSE_INT_VALUE:
            
            if (ps->lbcur == ps->lbend && !pref_GrowBuf(ps))
                return false; 
            if (isdigit(c))
                *ps->lbcur++ = c;
            else {
                *ps->lbcur++ = '\0'; 
                if (c == ')')
                    state = PREF_PARSE_UNTIL_SEMICOLON;
                else if (c == '/') { 
                    ps->nextstate = PREF_PARSE_UNTIL_CLOSE_PAREN;
                    state = PREF_PARSE_COMMENT_MAYBE_START;
                }
                else if (isspace(c))
                    state = PREF_PARSE_UNTIL_CLOSE_PAREN;
                else {
                    NS_WARNING("malformed pref file");
                    return false;
                }
            }
            break;

        
        case PREF_PARSE_COMMENT_MAYBE_START:
            switch (c) {
            case '*': 
                state = PREF_PARSE_COMMENT_BLOCK;
                break;
            case '/': 
                state = PREF_PARSE_UNTIL_EOL;
                break;
            default:
                
                NS_WARNING("malformed pref file");
                return false;
            }
            break;
        case PREF_PARSE_COMMENT_BLOCK:
            if (c == '*') 
                state = PREF_PARSE_COMMENT_BLOCK_MAYBE_END;
            break;
        case PREF_PARSE_COMMENT_BLOCK_MAYBE_END:
            switch (c) {
            case '/':
                state = ps->nextstate;
                ps->nextstate = PREF_PARSE_INIT;
                break;
            case '*':       
                break;
            default:
                state = PREF_PARSE_COMMENT_BLOCK;
            }
            break;

        
        case PREF_PARSE_ESC_SEQUENCE:
            


            switch (c) {
            case '\"':
            case '\'':
            case '\\':
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case 'x': 
            case 'u': 
                ps->esctmp[0] = c;
                ps->esclen = 1;
                ps->utf16[0] = ps->utf16[1] = 0;
                ps->sindex = (c == 'x' ) ?
                                HEX_ESC_NUM_DIGITS :
                                UTF16_ESC_NUM_DIGITS;
                state = PREF_PARSE_HEX_ESCAPE;
                continue;
            default:
                NS_WARNING("preserving unexpected JS escape sequence");
                

                if ((ps->lbcur+1) == ps->lbend && !pref_GrowBuf(ps))
                    return false; 
                *ps->lbcur++ = '\\'; 
                break;
            }
            *ps->lbcur++ = c;
            state = PREF_PARSE_QUOTED_STRING;
            break;

        
        case PREF_PARSE_HEX_ESCAPE:
            if ( c >= '0' && c <= '9' )
                udigit = (c - '0');
            else if ( c >= 'A' && c <= 'F' )
                udigit = (c - 'A') + 10;
            else if ( c >= 'a' && c <= 'f' )
                udigit = (c - 'a') + 10;
            else {
                
                NS_WARNING("preserving invalid or incomplete hex escape");
                *ps->lbcur++ = '\\';  
                if ((ps->lbcur + ps->esclen) >= ps->lbend && !pref_GrowBuf(ps))
                    return false;
                for (int i = 0; i < ps->esclen; ++i)
                    *ps->lbcur++ = ps->esctmp[i];

                
                
                --buf;
                state = PREF_PARSE_QUOTED_STRING;
                continue;
            }

            
            ps->esctmp[ps->esclen++] = c; 
            ps->utf16[1] <<= BITS_PER_HEX_DIGIT;
            ps->utf16[1] |= udigit;
            ps->sindex--;
            if (ps->sindex == 0) {
                
                int utf16len = 0;
                if (ps->utf16[0]) {
                    
                    utf16len = 2;
                }
                else if (0xD800 == (0xFC00 & ps->utf16[1])) {
                    
                    ps->utf16[0] = ps->utf16[1];
                    ps->utf16[1] = 0;
                    state = PREF_PARSE_UTF16_LOW_SURROGATE;
                    break;
                }
                else {
                    
                    ps->utf16[0] = ps->utf16[1];
                    utf16len = 1;
                }

                
                
                
                if (ps->lbcur+6 >= ps->lbend && !pref_GrowBuf(ps))
                    return false;

                ConvertUTF16toUTF8 converter(ps->lbcur);
                converter.write(ps->utf16, utf16len);
                ps->lbcur += converter.Size();
                state = PREF_PARSE_QUOTED_STRING;
            }
            break;

        
        case PREF_PARSE_UTF16_LOW_SURROGATE:
            if (ps->sindex == 0 && c == '\\') {
                ++ps->sindex;
            }
            else if (ps->sindex == 1 && c == 'u') {
                
                ps->sindex = UTF16_ESC_NUM_DIGITS;
                ps->esctmp[0] = 'u';
                ps->esclen = 1;
                state = PREF_PARSE_HEX_ESCAPE;
            }
            else {
                


                 --buf;
                 if (ps->sindex == 1)
                     state = PREF_PARSE_ESC_SEQUENCE;
                 else
                     state = PREF_PARSE_QUOTED_STRING;
                 continue;
            }
            break;

        
        case PREF_PARSE_UNTIL_OPEN_PAREN:
            
            if (c == '(')
                state = PREF_PARSE_UNTIL_NAME;
            else if (c == '/') {
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;
        case PREF_PARSE_UNTIL_CLOSE_PAREN:
            
            if (c == ')')
                state = PREF_PARSE_UNTIL_SEMICOLON;
            else if (c == '/') {
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;

        
        case PREF_PARSE_UNTIL_SEMICOLON:
            
            if (c == ';') {
                if (!pref_DoCallback(ps))
                    return false;
                state = PREF_PARSE_INIT;
            }
            else if (c == '/') {
                ps->nextstate = state; 
                state = PREF_PARSE_COMMENT_MAYBE_START;
            }
            else if (!isspace(c)) {
                NS_WARNING("malformed pref file");
                return false;
            }
            break;

        
        case PREF_PARSE_UNTIL_EOL:
            


            if (c == '\r' || c == '\n' || c == 0x1A) {
                state = ps->nextstate;
                ps->nextstate = PREF_PARSE_INIT; 
            }
            break;
        }
    }
    ps->state = state;
    return true;
}

#ifdef TEST_PREFREAD

static void
pref_reader(void       *closure, 
            const char *pref,
            PrefValue   val,
            PrefType    type,
            bool        defPref)
{
    printf("%spref(\"%s\", ", defPref ? "" : "user_", pref);
    switch (type) {
    case PREF_STRING:
        printf("\"%s\");\n", val.stringVal);
        break;
    case PREF_INT:
        printf("%i);\n", val.intVal);
        break;
    case PREF_BOOL:
        printf("%s);\n", val.boolVal == false ? "false" : "true");
        break;
    }
}

int
main(int argc, char **argv)
{
    PrefParseState ps;
    char buf[4096];     
    FILE *fp;
    int n;

    if (argc == 1) {
        printf("usage: prefread file.js\n");
        return -1;
    }

    fp = fopen(argv[1], "r");
    if (!fp) {
        printf("failed to open file\n");
        return -1;
    }

    PREF_InitParseState(&ps, pref_reader, nullptr);

    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        PREF_ParseBuf(&ps, buf, n);

    PREF_FinalizeParseState(&ps);

    fclose(fp);
    return 0;
}

#endif 
