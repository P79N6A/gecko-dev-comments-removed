






































#ifndef jsscan_h___
#define jsscan_h___



#include <stddef.h>
#include <stdio.h>
#include "jsconfig.h"
#include "jsopcode.h"
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

#define JS_KEYWORD(keyword, type, op, version) \
    extern const char js_##keyword##_str[];
#include "jskeyword.tbl"
#undef JS_KEYWORD

typedef enum JSTokenType {
    TOK_ERROR = -1,                     
    TOK_EOF = 0,                        
    TOK_EOL = 1,                        
    TOK_SEMI = 2,                       
    TOK_COMMA = 3,                      
    TOK_ASSIGN = 4,                     
    TOK_HOOK = 5, TOK_COLON = 6,        
    TOK_OR = 7,                         
    TOK_AND = 8,                        
    TOK_BITOR = 9,                      
    TOK_BITXOR = 10,                    
    TOK_BITAND = 11,                    
    TOK_EQOP = 12,                      
    TOK_RELOP = 13,                     
    TOK_SHOP = 14,                      
    TOK_PLUS = 15,                      
    TOK_MINUS = 16,                     
    TOK_STAR = 17, TOK_DIVOP = 18,      
    TOK_UNARYOP = 19,                   
    TOK_INC = 20, TOK_DEC = 21,         
    TOK_DOT = 22,                       
    TOK_LB = 23, TOK_RB = 24,           
    TOK_LC = 25, TOK_RC = 26,           
    TOK_LP = 27, TOK_RP = 28,           
    TOK_NAME = 29,                      
    TOK_NUMBER = 30,                    
    TOK_STRING = 31,                    
    TOK_OBJECT = 32,                    
    TOK_PRIMARY = 33,                   
    TOK_FUNCTION = 34,                  
    TOK_EXPORT = 35,                    
    TOK_IMPORT = 36,                    
    TOK_IF = 37,                        
    TOK_ELSE = 38,                      
    TOK_SWITCH = 39,                    
    TOK_CASE = 40,                      
    TOK_DEFAULT = 41,                   
    TOK_WHILE = 42,                     
    TOK_DO = 43,                        
    TOK_FOR = 44,                       
    TOK_BREAK = 45,                     
    TOK_CONTINUE = 46,                  
    TOK_IN = 47,                        
    TOK_VAR = 48,                       
    TOK_WITH = 49,                      
    TOK_RETURN = 50,                    
    TOK_NEW = 51,                       
    TOK_DELETE = 52,                    
    TOK_DEFSHARP = 53,                  
    TOK_USESHARP = 54,                  
    TOK_TRY = 55,                       
    TOK_CATCH = 56,                     
    TOK_FINALLY = 57,                   
    TOK_THROW = 58,                     
    TOK_INSTANCEOF = 59,                
    TOK_DEBUGGER = 60,                  
    TOK_XMLSTAGO = 61,                  
    TOK_XMLETAGO = 62,                  
    TOK_XMLPTAGC = 63,                  
    TOK_XMLTAGC = 64,                   
    TOK_XMLNAME = 65,                   
    TOK_XMLATTR = 66,                   
    TOK_XMLSPACE = 67,                  
    TOK_XMLTEXT = 68,                   
    TOK_XMLCOMMENT = 69,                
    TOK_XMLCDATA = 70,                  
    TOK_XMLPI = 71,                     
    TOK_AT = 72,                        
    TOK_DBLCOLON = 73,                  
    TOK_ANYNAME = 74,                   
    TOK_DBLDOT = 75,                    
    TOK_FILTER = 76,                    
    TOK_XMLELEM = 77,                   
    TOK_XMLLIST = 78,                   
    TOK_YIELD = 79,                     
    TOK_ARRAYCOMP = 80,                 
    TOK_ARRAYPUSH = 81,                 
    TOK_LEXICALSCOPE = 82,              
    TOK_LET = 83,                       
    TOK_BODY = 84,                      

    TOK_RESERVED,                       
    TOK_LIMIT                           
} JSTokenType;

#define IS_PRIMARY_TOKEN(tt) \
    ((uintN)((tt) - TOK_NAME) <= (uintN)(TOK_PRIMARY - TOK_NAME))

#define TOKEN_TYPE_IS_XML(tt) \
    (tt == TOK_AT || tt == TOK_DBLCOLON || tt == TOK_ANYNAME)

#if JS_HAS_BLOCK_SCOPE
# define TOKEN_TYPE_IS_DECL(tt) ((tt) == TOK_VAR || (tt) == TOK_LET)
#else
# define TOKEN_TYPE_IS_DECL(tt) ((tt) == TOK_VAR)
#endif

struct JSStringBuffer {
    jschar      *base;
    jschar      *limit;         
    jschar      *ptr;           
    void        *data;
    JSBool      (*grow)(JSStringBuffer *sb, size_t newlength);
    void        (*free)(JSStringBuffer *sb);
};

#define STRING_BUFFER_ERROR_BASE        ((jschar *) 1)
#define STRING_BUFFER_OK(sb)            ((sb)->base != STRING_BUFFER_ERROR_BASE)
#define STRING_BUFFER_OFFSET(sb)        ((sb)->ptr -(sb)->base)

extern void
js_InitStringBuffer(JSStringBuffer *sb);

extern void
js_FinishStringBuffer(JSStringBuffer *sb);

extern void
js_AppendChar(JSStringBuffer *sb, jschar c);

extern void
js_RepeatChar(JSStringBuffer *sb, jschar c, uintN count);

extern void
js_AppendCString(JSStringBuffer *sb, const char *asciiz);

extern void
js_AppendJSString(JSStringBuffer *sb, JSString *str);

struct JSTokenPtr {
    uint16              index;          
    uint16              lineno;         
};

struct JSTokenPos {
    JSTokenPtr          begin;          
    JSTokenPtr          end;            
};

struct JSToken {
    JSTokenType         type;           
    JSTokenPos          pos;            
    jschar              *ptr;           
    union {
        struct {                        
            JSOp        op;             
            JSAtom      *atom;          
        } s;
        struct {                        
            JSOp        op;             
            JSParsedObjectBox *pob;     
        } o;
        struct {                        
            JSAtom      *atom2;         
            JSAtom      *atom;          
        } p;
        jsdouble        dval;           
    } u;
};

#define t_op            u.s.op
#define t_pob           u.o.pob
#define t_atom          u.s.atom
#define t_atom2         u.p.atom2
#define t_dval          u.dval

typedef struct JSTokenBuf {
    jschar              *base;          
    jschar              *limit;         
    jschar              *ptr;           
} JSTokenBuf;

#define JS_LINE_LIMIT   256             /* logical line buffer size limit --
                                           physical line length is unlimited */
#define NTOKENS         4               /* 1 current + 2 lookahead, rounded */
#define NTOKENS_MASK    (NTOKENS-1)     /* to power of 2 to avoid divmod by 3 */

struct JSTokenStream {
    JSToken             tokens[NTOKENS];
    uintN               cursor;         
    uintN               lookahead;      
    uintN               lineno;         
    uintN               ungetpos;       
    jschar              ungetbuf[6];    
    uintN               flags;          
    ptrdiff_t           linelen;        
    ptrdiff_t           linepos;        
    JSTokenBuf          linebuf;        
    JSTokenBuf          userbuf;        
    JSStringBuffer      tokenbuf;       
    const char          *filename;      
    FILE                *file;          
    JSPrincipals        *principals;    
    JSSourceHandler     listener;       
    void                *listenerData;  
    void                *listenerTSData;
    jschar              *saveEOL;       

    JSParseContext      *parseContext;
};

#define CURRENT_TOKEN(ts)       ((ts)->tokens[(ts)->cursor])
#define ON_CURRENT_LINE(ts,pos) ((uint16)(ts)->lineno == (pos).end.lineno)


#define TSF_ERROR       0x01            /* fatal error while compiling */
#define TSF_EOF         0x02            /* hit end of file */
#define TSF_NEWLINES    0x04            /* tokenize newlines */
#define TSF_OPERAND     0x08            /* looking for operand, not operator */
#define TSF_NLFLAG      0x20            /* last linebuf ended with \n */
#define TSF_CRFLAG      0x40            /* linebuf would have ended with \r */
#define TSF_DIRTYLINE   0x80            /* non-whitespace since start of line */
#define TSF_OWNFILENAME 0x100           /* ts->filename is malloc'd */
#define TSF_XMLTAGMODE  0x200           /* scanning within an XML tag in E4X */
#define TSF_XMLTEXTMODE 0x400           /* scanning XMLText terminal from E4X */
#define TSF_XMLONLYMODE 0x800           /* don't scan {expr} within text/tag */


#define TSF_UNEXPECTED_EOF 0x1000




















#define TSF_IN_HTML_COMMENT 0x2000


#define TSF_KEYWORD_IS_NAME 0x4000


#define LINE_SEPARATOR  0x2028
#define PARA_SEPARATOR  0x2029









extern JSTokenStream *
js_NewTokenStream(JSContext *cx, const jschar *base, size_t length,
                  const char *filename, uintN lineno, JSPrincipals *principals);

extern JS_FRIEND_API(JSTokenStream *)
js_NewBufferTokenStream(JSContext *cx, const jschar *base, size_t length);

extern JS_FRIEND_API(JSTokenStream *)
js_NewFileTokenStream(JSContext *cx, const char *filename, FILE *defaultfp);

extern JS_FRIEND_API(JSBool)
js_CloseTokenStream(JSContext *cx, JSTokenStream *ts);

extern JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file);





extern JSTokenType
js_CheckKeyword(const jschar *chars, size_t length);





extern JS_FRIEND_API(void)
js_MapKeywords(void (*mapfun)(const char *));





extern JSBool
js_IsIdentifier(JSString *str);





extern JSBool
js_ReportCompileErrorNumber(JSContext *cx, void *handle, uintN flags,
                            uintN errorNumber, ...);

extern JSBool
js_ReportCompileErrorNumberUC(JSContext *cx, void *handle, uintN flags,
                              uintN errorNumber, ...);


#define JSREPORT_HANDLE 0x300
#define JSREPORT_TS     0x000
#define JSREPORT_CG     0x100
#define JSREPORT_PN     0x200




extern JSTokenType
js_PeekToken(JSContext *cx, JSTokenStream *ts);

extern JSTokenType
js_PeekTokenSameLine(JSContext *cx, JSTokenStream *ts);




extern JSTokenType
js_GetToken(JSContext *cx, JSTokenStream *ts);




extern void
js_UngetToken(JSTokenStream *ts);




extern JSBool
js_MatchToken(JSContext *cx, JSTokenStream *ts, JSTokenType tt);

JS_END_EXTERN_C

#endif 
