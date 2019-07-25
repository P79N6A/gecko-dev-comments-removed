






































#ifndef jsscan_h___
#define jsscan_h___



#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "jsversion.h"
#include "jsopcode.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsvector.h"

#define JS_KEYWORD(keyword, type, op, version) \
    extern const char js_##keyword##_str[];
#include "jskeyword.tbl"
#undef JS_KEYWORD

namespace js {

enum TokenKind {
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
    TOK_REGEXP = 32,                    
    TOK_PRIMARY = 33,                   
    TOK_FUNCTION = 34,                  
    TOK_IF = 35,                        
    TOK_ELSE = 36,                      
    TOK_SWITCH = 37,                    
    TOK_CASE = 38,                      
    TOK_DEFAULT = 39,                   
    TOK_WHILE = 40,                     
    TOK_DO = 41,                        
    TOK_FOR = 42,                       
    TOK_BREAK = 43,                     
    TOK_CONTINUE = 44,                  
    TOK_IN = 45,                        
    TOK_VAR = 46,                       
    TOK_WITH = 47,                      
    TOK_RETURN = 48,                    
    TOK_NEW = 49,                       
    TOK_DELETE = 50,                    
    TOK_DEFSHARP = 51,                  
    TOK_USESHARP = 52,                  
    TOK_TRY = 53,                       
    TOK_CATCH = 54,                     
    TOK_FINALLY = 55,                   
    TOK_THROW = 56,                     
    TOK_INSTANCEOF = 57,                
    TOK_DEBUGGER = 58,                  
    TOK_XMLSTAGO = 59,                  
    TOK_XMLETAGO = 60,                  
    TOK_XMLPTAGC = 61,                  
    TOK_XMLTAGC = 62,                   
    TOK_XMLNAME = 63,                   
    TOK_XMLATTR = 64,                   
    TOK_XMLSPACE = 65,                  
    TOK_XMLTEXT = 66,                   
    TOK_XMLCOMMENT = 67,                
    TOK_XMLCDATA = 68,                  
    TOK_XMLPI = 69,                     
    TOK_AT = 70,                        
    TOK_DBLCOLON = 71,                  
    TOK_ANYNAME = 72,                   
    TOK_DBLDOT = 73,                    
    TOK_FILTER = 74,                    
    TOK_XMLELEM = 75,                   
    TOK_XMLLIST = 76,                   
    TOK_YIELD = 77,                     
    TOK_ARRAYCOMP = 78,                 
    TOK_ARRAYPUSH = 79,                 
    TOK_LEXICALSCOPE = 80,              
    TOK_LET = 81,                       
    TOK_SEQ = 82,                       

    TOK_FORHEAD = 83,                   
    TOK_ARGSBODY = 84,                  
    TOK_UPVARS = 85,                    


    TOK_RESERVED,                       
    TOK_STRICT_RESERVED,                
    TOK_LIMIT                           
};

static inline bool
TokenKindIsXML(TokenKind tt)
{
    return tt == TOK_AT || tt == TOK_DBLCOLON || tt == TOK_ANYNAME;
}

static inline bool
TreeTypeIsXML(TokenKind tt)
{
    return tt == TOK_XMLCOMMENT || tt == TOK_XMLCDATA || tt == TOK_XMLPI ||
           tt == TOK_XMLELEM || tt == TOK_XMLLIST;
}

static inline bool
TokenKindIsDecl(TokenKind tt)
{
#if JS_HAS_BLOCK_SCOPE
    return tt == TOK_VAR || tt == TOK_LET;
#else
    return tt == TOK_VAR;
#endif
}

struct TokenPtr {
    uint32              index;          
    uint32              lineno;         

    bool operator==(const TokenPtr& bptr) {
        return index == bptr.index && lineno == bptr.lineno;
    }

    bool operator!=(const TokenPtr& bptr) {
        return index != bptr.index || lineno != bptr.lineno;
    }

    bool operator <(const TokenPtr& bptr) {
        return lineno < bptr.lineno ||
               (lineno == bptr.lineno && index < bptr.index);
    }

    bool operator <=(const TokenPtr& bptr) {
        return lineno < bptr.lineno ||
               (lineno == bptr.lineno && index <= bptr.index);
    }

    bool operator >(const TokenPtr& bptr) {
        return !(*this <= bptr);
    }

    bool operator >=(const TokenPtr& bptr) {
        return !(*this < bptr);
    }
};

struct TokenPos {
    TokenPtr          begin;          
    TokenPtr          end;            

    bool operator==(const TokenPos& bpos) {
        return begin == bpos.begin && end == bpos.end;
    }

    bool operator!=(const TokenPos& bpos) {
        return begin != bpos.begin || end != bpos.end;
    }

    bool operator <(const TokenPos& bpos) {
        return begin < bpos.begin;
    }

    bool operator <=(const TokenPos& bpos) {
        return begin <= bpos.begin;
    }

    bool operator >(const TokenPos& bpos) {
        return !(*this <= bpos);
    }

    bool operator >=(const TokenPos& bpos) {
        return !(*this < bpos);
    }
};

struct Token {
    TokenKind           type;           
    TokenPos            pos;            
    const jschar        *ptr;           
    union {
        struct {                        
            JSOp        op;             
            JSAtom      *atom;          
        } s;
        uintN           reflags;        

        struct {                        
            JSAtom      *atom2;         
            JSAtom      *atom;          
        } p;
        jsdouble        dval;           
    } u;
};

enum TokenStreamFlags
{
    TSF_EOF = 0x02,             
    TSF_EOL = 0x04,             
    TSF_OPERAND = 0x08,         
    TSF_UNEXPECTED_EOF = 0x10,  
    TSF_KEYWORD_IS_NAME = 0x20, 
    TSF_STRICT_MODE_CODE = 0x40,
    TSF_DIRTYLINE = 0x80,       
    TSF_OWNFILENAME = 0x100,    
    TSF_XMLTAGMODE = 0x200,     
    TSF_XMLTEXTMODE = 0x400,    
    TSF_XMLONLYMODE = 0x800,    
    TSF_OCTAL_CHAR = 0x1000,    

    


















    TSF_IN_HTML_COMMENT = 0x2000
};

#define t_op            u.s.op
#define t_reflags       u.reflags
#define t_atom          u.s.atom
#define t_atom2         u.p.atom2
#define t_dval          u.dval

class TokenStream
{
    
    enum {
        LINE_SEPARATOR = 0x2028,
        PARA_SEPARATOR = 0x2029
    };

    static const size_t ntokens = 4;                

    static const uintN ntokensMask = ntokens - 1;

  public:
    typedef Vector<jschar, 32> CharBuffer;

    









    TokenStream(JSContext *);

    



    bool init(const jschar *base, size_t length, const char *filename, uintN lineno,
              JSVersion version);
    ~TokenStream();

    
    JSContext *getContext() const { return cx; }
    bool onCurrentLine(const TokenPos &pos) const { return lineno == pos.end.lineno; }
    const Token &currentToken() const { return tokens[cursor]; }
    bool isCurrentTokenType(TokenKind type) const {
        return currentToken().type == type;
    }
    bool isCurrentTokenType(TokenKind type1, TokenKind type2) const {
        TokenKind type = currentToken().type;
        return type == type1 || type == type2;
    }
    const CharBuffer &getTokenbuf() const { return tokenbuf; }
    const char *getFilename() const { return filename; }
    uintN getLineno() const { return lineno; }
    
    JSVersion versionNumber() const { return VersionNumber(version); }
    JSVersion versionWithFlags() const { return version; }
    bool hasAnonFunFix() const { return VersionHasAnonFunFix(version); }
    bool hasXML() const { return xml || VersionShouldParseXML(versionNumber()); }
    void setXML(bool enabled) { xml = enabled; }

    
    void setStrictMode(bool enabled = true) { setFlag(enabled, TSF_STRICT_MODE_CODE); }
    void setXMLTagMode(bool enabled = true) { setFlag(enabled, TSF_XMLTAGMODE); }
    void setXMLOnlyMode(bool enabled = true) { setFlag(enabled, TSF_XMLONLYMODE); }
    void setUnexpectedEOF(bool enabled = true) { setFlag(enabled, TSF_UNEXPECTED_EOF); }
    void setOctalCharacterEscape(bool enabled = true) { setFlag(enabled, TSF_OCTAL_CHAR); }

    bool isStrictMode() { return !!(flags & TSF_STRICT_MODE_CODE); }
    bool isXMLTagMode() { return !!(flags & TSF_XMLTAGMODE); }
    bool isXMLOnlyMode() { return !!(flags & TSF_XMLONLYMODE); }
    bool isUnexpectedEOF() { return !!(flags & TSF_UNEXPECTED_EOF); }
    bool isEOF() const { return !!(flags & TSF_EOF); }
    bool hasOctalCharacterEscape() const { return flags & TSF_OCTAL_CHAR; }

    
    bool reportCompileErrorNumberVA(JSParseNode *pn, uintN flags, uintN errorNumber, va_list ap);
    void mungeCurrentToken(TokenKind newKind) { tokens[cursor].type = newKind; }
    void mungeCurrentToken(JSOp newOp) { tokens[cursor].t_op = newOp; }
    void mungeCurrentToken(TokenKind newKind, JSOp newOp) {
        mungeCurrentToken(newKind);
        mungeCurrentToken(newOp);
    }

  private:
    static JSAtom *atomize(JSContext *cx, CharBuffer &cb);
    bool putIdentInTokenbuf(const jschar *identStart);

    



    class Flagger {
        TokenStream * const parent;
        uintN       flags;
      public:
        Flagger(TokenStream *parent, uintN withFlags) : parent(parent), flags(withFlags) {
            parent->flags |= flags;
        }

        ~Flagger() { parent->flags &= ~flags; }
    };
    friend class Flagger;

    void setFlag(bool enabled, TokenStreamFlags flag) {
        if (enabled)
            flags |= flag;
        else
            flags &= ~flag;
    }

  public:
    



    TokenKind getToken() {
        
        if (lookahead != 0) {
            JS_ASSERT(!(flags & TSF_XMLTEXTMODE));
            lookahead--;
            cursor = (cursor + 1) & ntokensMask;
            TokenKind tt = currentToken().type;
            JS_ASSERT(tt != TOK_EOL);
            return tt;
        }

        return getTokenInternal();
    }

    
    TokenKind getToken(uintN withFlags) {
        Flagger flagger(this, withFlags);
        return getToken();
    }

    


    void ungetToken() {
        JS_ASSERT(lookahead < ntokensMask);
        lookahead++;
        cursor = (cursor - 1) & ntokensMask;
    }

    TokenKind peekToken(uintN withFlags = 0) {
        Flagger flagger(this, withFlags);
        if (lookahead != 0) {
            JS_ASSERT(lookahead == 1);
            return tokens[(cursor + lookahead) & ntokensMask].type;
        }
        TokenKind tt = getToken();
        ungetToken();
        return tt;
    }

    TokenKind peekTokenSameLine(uintN withFlags = 0) {
        if (!onCurrentLine(currentToken().pos))
            return TOK_EOL;

        if (lookahead != 0) {
            JS_ASSERT(lookahead == 1);
            return tokens[(cursor + lookahead) & ntokensMask].type;
        }

        



        flags &= ~TSF_EOL;
        TokenKind tt = getToken(withFlags);
        if (flags & TSF_EOL) {
            tt = TOK_EOL;
            flags &= ~TSF_EOL;
        }
        ungetToken();
        return tt;
    }

    


    bool matchToken(TokenKind tt, uintN withFlags = 0) {
        Flagger flagger(this, withFlags);
        if (getToken() == tt)
            return true;
        ungetToken();
        return false;
    }

  private:
    






    class TokenBuf {
      public:
        TokenBuf() : base(NULL), limit(NULL), ptr(NULL) { }

        void init(const jschar *buf, size_t length) {
            base = ptr = buf;
            limit = base + length;
        }

        bool hasRawChars() const {
            return ptr < limit;
        }

        bool atStart() const {
            return ptr == base;
        }

        jschar getRawChar() {
            return *ptr++;      
        }

        jschar peekRawChar() const {
            return *ptr;        
        }

        bool matchRawChar(jschar c) {
            if (*ptr == c) {    
                ptr++;
                return true;
            }
            return false;
        }

        bool matchRawCharBackwards(jschar c) {
            JS_ASSERT(ptr);     
            if (*(ptr - 1) == c) {
                ptr--;
                return true;
            }
            return false;
        }

        void ungetRawChar() {
            JS_ASSERT(ptr);     
            ptr--;
        }

        const jschar *addressOfNextRawChar() {
            JS_ASSERT(ptr);     
            return ptr;
        }

        
        void setAddressOfNextRawChar(const jschar *a) {
            JS_ASSERT(a);
            ptr = a;
        }

#ifdef DEBUG
        




        void poison() {
            ptrWhenPoisoned = ptr;
            ptr = NULL;
        }
#endif

        static bool isRawEOLChar(int32 c) {
            return (c == '\n' || c == '\r' || c == LINE_SEPARATOR || c == PARA_SEPARATOR);
        }

        const jschar *findEOL();

      private:
        const jschar *base;             
        const jschar *limit;            
        const jschar *ptr;              
        const jschar *ptrWhenPoisoned;  
    };

    TokenKind getTokenInternal();     

    int32 getChar();
    int32 getCharIgnoreEOL();
    void ungetChar(int32 c);
    void ungetCharIgnoreEOL(int32 c);
    Token *newToken(ptrdiff_t adjust);
    bool peekUnicodeEscape(int32 *c);
    bool matchUnicodeEscapeIdStart(int32 *c);
    bool matchUnicodeEscapeIdent(int32 *c);
    bool peekChars(intN n, jschar *cp);
    bool getAtLine();

    bool getXMLEntity();
    bool getXMLTextOrTag(TokenKind *ttp, Token **tpp);
    bool getXMLMarkup(TokenKind *ttp, Token **tpp);

    bool matchChar(int32 expect) {
        int32 c = getChar();
        if (c == expect)
            return true;
        ungetChar(c);
        return false;
    }

    int32 peekChar() {
        int32 c = getChar();
        ungetChar(c);
        return c;
    }

    void skipChars(intN n) {
        while (--n >= 0)
            getChar();
    }

    void updateLineInfoForEOL();
    void updateFlagsForEOL();

    JSContext           * const cx;
    Token               tokens[ntokens];
    uintN               cursor;         
    uintN               lookahead;      
    uintN               lineno;         
    uintN               flags;          
    const jschar        *linebase;      
    const jschar        *prevLinebase;  
    TokenBuf            userbuf;        
    const char          *filename;      
    void                *listenerTSData;
    CharBuffer          tokenbuf;       
    int8                oneCharTokens[128];  
    JSPackedBool        maybeEOL[256];       
    JSPackedBool        maybeStrSpecial[256];
    JSVersion           version;        
    bool                xml;            
};

} 

extern void
js_CloseTokenStream(JSContext *cx, js::TokenStream *ts);

extern JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file);

namespace js {

struct KeywordInfo {
    const char  *chars;         
    TokenKind   tokentype;
    JSOp        op;             
    JSVersion   version;        
};





extern const KeywordInfo *
FindKeyword(const jschar *s, size_t length);

} 





typedef void (*JSMapKeywordFun)(const char *);





extern JSBool
js_IsIdentifier(JSLinearString *str);





#define JSREPORT_UC 0x100

namespace js {






bool
ReportCompileErrorNumber(JSContext *cx, TokenStream *ts, JSParseNode *pn, uintN flags,
                         uintN errorNumber, ...);


















bool
ReportStrictModeError(JSContext *cx, TokenStream *ts, JSTreeContext *tc, JSParseNode *pn,
                      uintN errorNumber, ...);

} 

#endif 
