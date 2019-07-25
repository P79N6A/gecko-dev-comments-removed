














































#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
    #include <ieeefp.h>
#else
    #define isinff(x) (((*(int *)&(x) & 0x7f800000L)==0x7f800000L) && \
                       ((*(int *)&(x) & 0x007fffffL)==0000000000L))
#endif

#include "compiler/preprocessor/slglobals.h"
#include "compiler/util.h"

typedef struct StringInputSrc {
    InputSrc base;
    char *p;
} StringInputSrc;

static int eof_scan(InputSrc *is, yystypepp * yylvalpp)
{
    return EOF;
} 

static void noop(InputSrc *in, int ch, yystypepp * yylvalpp) {}

static InputSrc eof_inputsrc = { 0, &eof_scan, &eof_scan, &noop };

static int byte_scan(InputSrc *, yystypepp * yylvalpp);

#define EOL_SY '\n'

#if defined(_MSC_VER)
    #define DBG_BREAKPOINT() __asm int 3
#elif defined(_M_AMD64)
    #define DBG_BREAKPOINT() assert(!"Dbg_Breakpoint");
#else
    #define DBG_BREAKPOINT()
#endif

#if defined(_MSC_VER) && !defined(_M_AMD64)
    __int64 RDTSC ( void ) {

        __int64 v;
    
        __asm __emit 0x0f
        __asm __emit 0x31
        __asm mov dword ptr v, eax
        __asm mov dword ptr v+4, edx
    
        return v;
    }
#endif


int InitScanner(CPPStruct *cpp)
{
    
    if (!InitCPP())
        return 0;

    cpp->mostRecentToken = 0;
    cpp->tokenLoc = &cpp->ltokenLoc;

    cpp->ltokenLoc.file = 0;
    cpp->ltokenLoc.line = 0;

    cpp->currentInput = &eof_inputsrc;
    cpp->previous_token = '\n';
    cpp->pastFirstStatement = 0;

    return 1;
} 

int FreeScanner(void)
{
    return (FreeCPP());
}







static int str_getch(StringInputSrc *in)
{
    for(;;){
       if (*in->p){
          if (*in->p == '\n') {
             in->base.line++;
             IncLineNumber();
          }
          return *in->p++;
       }
       if(++(cpp->PaWhichStr) < cpp->PaArgc){
          free(in);
          SetStringNumber(cpp->PaWhichStr);
          SetLineNumber(1);
          ScanFromString(cpp->PaArgv[cpp->PaWhichStr]);
          in=(StringInputSrc*)cpp->currentInput;
          continue;             
       }
       else{
          cpp->currentInput = in->base.prev;
          cpp->PaWhichStr=0;
          free(in);
          return EOF;
       }  
    }
} 

static void str_ungetch(StringInputSrc *in, int ch, yystypepp *type) {
    if (in->p[-1] == ch)in->p--;
    else {
        *(in->p)='\0'; 
        cpp->PaWhichStr--;
    }  
    if (ch == '\n') {
        in->base.line--;
        DecLineNumber();
    }
} 

int ScanFromString(const char *s)
{
    
    StringInputSrc *in = malloc(sizeof(StringInputSrc));
    memset(in, 0, sizeof(StringInputSrc));
    in->p = (char*) s;
    in->base.line = 1;
    in->base.scan = byte_scan;
    in->base.getch = (int (*)(InputSrc *, yystypepp *))str_getch;
    in->base.ungetch = (void (*)(InputSrc *, int, yystypepp *))str_ungetch;
    in->base.prev = cpp->currentInput;
    cpp->currentInput = &in->base;

    return 1;
} 






#define APPEND_CHAR_S(ch, str, len, max_len) \
      if (len < max_len) { \
          str[len++] = ch; \
      } else if (!alreadyComplained) { \
          CPPErrorToInfoLog("BUFFER OVERFLOW"); \
          alreadyComplained = 1; \
      }









static int lFloatConst(int ch, int len, yystypepp * yylvalpp)
{
    int alreadyComplained = 0;
    assert((ch == '.') || (ch == 'e') || (ch == 'E'));

    if (ch == '.') {
        do {
            APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
        } while (ch >= '0' && ch <= '9');
    }

    
    if (ch == 'e' || ch == 'E') {
        APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
        ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
        if (ch == '+') {
            APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
        } else if (ch == '-') {
            APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
        }
        if (ch >= '0' && ch <= '9') {
            while (ch >= '0' && ch <= '9') {
                APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            }
        } else {
            CPPErrorToInfoLog("EXPONENT INVALID");
        }
    }
    cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);

    assert(len <= MAX_SYMBOL_NAME_LEN);
    yylvalpp->symbol_name[len] = '\0';
    yylvalpp->sc_fval = (float) atof_dot(yylvalpp->symbol_name);
    if (isinff(yylvalpp->sc_fval)) {
        CPPErrorToInfoLog("FLOAT CONSTANT OVERFLOW");
    }
    return CPP_FLOATCONSTANT;
} 




    
static int byte_scan(InputSrc *in, yystypepp * yylvalpp)
{
    char string_val[MAX_STRING_LEN + 1];
    int alreadyComplained = 0;
    int len, ch, ii, ival = 0;

    for (;;) {
        yylvalpp->sc_int = 0;
        ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
 
        while (ch == ' ' || ch == '\t' || ch == '\r') {
            yylvalpp->sc_int = 1;
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
        }
        
        cpp->ltokenLoc.file = cpp->currentInput->name;
        cpp->ltokenLoc.line = cpp->currentInput->line;
        alreadyComplained = 0;
        len = 0;
        switch (ch) {
        default:
            return ch; 
        case EOF:
            return -1;
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z': case '_':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':            
            do {
                APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            } while ((ch >= 'a' && ch <= 'z') ||
                     (ch >= 'A' && ch <= 'Z') ||
                     (ch >= '0' && ch <= '9') ||
                     ch == '_');
            assert(len <= MAX_SYMBOL_NAME_LEN);
            yylvalpp->symbol_name[len] = '\0';
            cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
            yylvalpp->sc_ident = LookUpAddString(atable, yylvalpp->symbol_name);
            return CPP_IDENTIFIER;
            break;
        case '0':
            APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == 'x' || ch == 'X') {  
                APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                if ((ch >= '0' && ch <= '9') ||
                    (ch >= 'A' && ch <= 'F') ||
                    (ch >= 'a' && ch <= 'f'))
                {
                    ival = 0;
                    do {
                        if ((ival <= 0x0fffffff) && (len < MAX_SYMBOL_NAME_LEN)) {
                            yylvalpp->symbol_name[len++] = ch;
                            if (ch >= '0' && ch <= '9') {
                                ii = ch - '0';
                            } else if (ch >= 'A' && ch <= 'F') {
                                ii = ch - 'A' + 10;
                            } else {
                                ii = ch - 'a' + 10;
                            }
                            ival = (ival << 4) | ii;
                        } else if (!alreadyComplained) {
                            CPPErrorToInfoLog("HEX CONSTANT OVERFLOW");
                            alreadyComplained = 1;
                        }
                        ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                    } while ((ch >= '0' && ch <= '9') ||
                             (ch >= 'A' && ch <= 'F') ||
                             (ch >= 'a' && ch <= 'f'));
                } else {
                    CPPErrorToInfoLog("HEX CONSTANT INVALID");
                }
                assert(len <= MAX_SYMBOL_NAME_LEN);
                yylvalpp->symbol_name[len] = '\0';
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                yylvalpp->sc_int = ival;
                return CPP_INTCONSTANT;
            } else if (ch >= '0' && ch <= '7') { 
                ival = 0;
                do {
                    if ((ival <= 0x1fffffff) && (len < MAX_SYMBOL_NAME_LEN)) {
                        yylvalpp->symbol_name[len++] = ch;
                        ii = ch - '0';
                        ival = (ival << 3) | ii;
                    } else if (!alreadyComplained) {
                        CPPErrorToInfoLog("OCT CONSTANT OVERFLOW");
                        alreadyComplained = 1;
                    }
                    ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                } while (ch >= '0' && ch <= '7');
                if (ch == '.' || ch == 'e' || ch == 'f' || ch == 'h' || ch == 'x'|| ch == 'E') 
                     return lFloatConst(ch, len, yylvalpp);
                assert(len <= MAX_SYMBOL_NAME_LEN);
                yylvalpp->symbol_name[len] = '\0';
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                yylvalpp->sc_int = ival;
                return CPP_INTCONSTANT;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                ch = '0';
            }
            
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            do {
                APPEND_CHAR_S(ch, yylvalpp->symbol_name, len, MAX_SYMBOL_NAME_LEN);
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            } while (ch >= '0' && ch <= '9');
            if (ch == '.' || ch == 'e' || ch == 'E') {
                return lFloatConst(ch, len, yylvalpp);
            } else {
                assert(len <= MAX_SYMBOL_NAME_LEN);
                yylvalpp->symbol_name[len] = '\0';
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                ival = 0;
                for (ii = 0; ii < len; ii++) {
                    ch = yylvalpp->symbol_name[ii] - '0';
                    ival = ival*10 + ch;
                    if ((ival > 214748364) || (ival == 214748364 && ch >= 8)) {
                        CPPErrorToInfoLog("INTEGER CONSTANT OVERFLOW");
                        break;
                    }
                }
                yylvalpp->sc_int = ival;
                if(ival==0)
                   strcpy(yylvalpp->symbol_name,"0");
                return CPP_INTCONSTANT;
            }
            break;
        case '-':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '-') {
                return CPP_DEC_OP;
            } else if (ch == '=') {
                return CPP_SUB_ASSIGN;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '-';
            }
        case '+':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '+') {
                return CPP_INC_OP;
            } else if (ch == '=') {
                return CPP_ADD_ASSIGN;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '+';
            }
        case '*':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '=') {
                return CPP_MUL_ASSIGN;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '*';
            }
        case '%':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '=') {
                return CPP_MOD_ASSIGN;
            } else if (ch == '>'){
                return CPP_RIGHT_BRACE;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '%';
            }
        case ':':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '>') {
                return CPP_RIGHT_BRACKET;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return ':';
            }
        case '^':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '^') {
                return CPP_XOR_OP;
            } else {
                if (ch == '=')
                    return CPP_XOR_ASSIGN;
                else{
                  cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                  return '^';
                }
            }
        
        case '=':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '=') {
                return CPP_EQ_OP;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '=';
            }
        case '!':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '=') {
                return CPP_NE_OP;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '!';
            }
        case '|':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '|') {
                return CPP_OR_OP;
            } else {
                if (ch == '=')
                    return CPP_OR_ASSIGN;
                else{
                  cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                  return '|';
                }
            }
        case '&':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '&') {
                return CPP_AND_OP;
            } else {
                if (ch == '=')
                    return CPP_AND_ASSIGN;
                else{
                  cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                  return '&';
                }
            }
        case '<':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '<') {
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                if(ch == '=')
                    return CPP_LEFT_ASSIGN;
                else{
                    cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                    return CPP_LEFT_OP;
                }
            } else {
                if (ch == '=') {
                    return CPP_LE_OP;
                } else {
                    if (ch == '%')
                        return CPP_LEFT_BRACE;
                    else if (ch == ':')
                        return CPP_LEFT_BRACKET;
                    else{
                        cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                        return '<';
                    }
                }
            }
        case '>':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '>') {
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                if(ch == '=')
                    return CPP_RIGHT_ASSIGN;
                else{
                    cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                    return CPP_RIGHT_OP;
                }
            } else {
                if (ch == '=') {
                    return CPP_GE_OP;
                } else {
                    cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                    return '>';
                }
            }
        case '.':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch >= '0' && ch <= '9') {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return lFloatConst('.', 0, yylvalpp);
            } else {
                if (ch == '.') {
                    return -1; 
                } else {
                    cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                    return '.';
                }
            }
        case '/':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            if (ch == '/') {
                do {
                    ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                } while (ch != '\n' && ch != EOF);
                if (ch == EOF)
                    return -1;
                return '\n';
            } else if (ch == '*') {
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                do {
                    while (ch != '*') {
                        if (ch == EOF) {
                            CPPErrorToInfoLog("EOF IN COMMENT");
                            return -1;
                        }
                        ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                    }
                    ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
                    if (ch == EOF) {
                        CPPErrorToInfoLog("EOF IN COMMENT");
                        return -1;
                    }
                } while (ch != '/');
                
            } else if (ch == '=') {
                return CPP_DIV_ASSIGN;
            } else {
                cpp->currentInput->ungetch(cpp->currentInput, ch, yylvalpp);
                return '/';
            }
            break;
        case '"':
            ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            while (ch != '"' && ch != '\n' && ch != EOF) {
                if (ch == '\\') {
                    CPPErrorToInfoLog("The line continuation character (\\) is not part of the OpenGL ES Shading Language");
                    return -1;
                }
                APPEND_CHAR_S(ch, string_val, len, MAX_STRING_LEN);
                ch = cpp->currentInput->getch(cpp->currentInput, yylvalpp);
            };
            assert(len <= MAX_STRING_LEN);
            string_val[len] = '\0';
            if (ch == '"') {
                yylvalpp->sc_ident = LookUpAddString(atable, string_val);
                return CPP_STRCONSTANT;
            } else {
                CPPErrorToInfoLog("EOL IN STRING");
                return ERROR_SY;
            }
            break;
        }
    }
} 

int yylex_CPP(char* buf, int maxSize)
{    
    yystypepp yylvalpp;
    int token = '\n';   

    for(;;) {

        char* tokenString = 0;
        token = cpp->currentInput->scan(cpp->currentInput, &yylvalpp);
        if(check_EOF(token))
            return 0;
        if (token < 0) {
            
            
            CPPErrorToInfoLog("preprocessor encountered non-ASCII character in shader source");
            return 0;
        }
        if (token == '#') {
            if (cpp->previous_token == '\n'|| cpp->previous_token == 0) {
                token = readCPPline(&yylvalpp);
                if(check_EOF(token))
                    return 0;
                continue;
            } else {
                CPPErrorToInfoLog("preprocessor command must not be preceded by any other statement in that line");
                return 0;
            }
        }
        cpp->previous_token = token;
        
        if (token == CPP_IDENTIFIER && MacroExpand(yylvalpp.sc_ident, &yylvalpp)) {
            cpp->pastFirstStatement = 1;
            continue;
        }

        if (token == '\n')
            continue;
        cpp->pastFirstStatement = 1;

        if (token == CPP_IDENTIFIER) {
            tokenString = GetStringOfAtom(atable,yylvalpp.sc_ident);
        } else if (token == CPP_FLOATCONSTANT || token == CPP_INTCONSTANT){
            tokenString = yylvalpp.symbol_name;
        } else {
            tokenString = GetStringOfAtom(atable,token);
        }

        if (tokenString) {
            int len = strlen(tokenString);
            cpp->tokensBeforeEOF = 1;
            if (len >= maxSize) {
                return maxSize;
            } else  if (len > 0) {
                strcpy(buf, tokenString);
                return len;
            }

            return 0;
        }
    }

    return 0;
} 


int check_EOF(int token)
{
   if(token==-1){
       if(cpp->ifdepth >0){
        CPPErrorToInfoLog("#endif missing!! Compilation stopped");
        cpp->CompileError=1;
       }
      return 1;
   }
   return 0;
}





