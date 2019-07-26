





#ifndef TokenStream_h__
#define TokenStream_h__




#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "jscntxt.h"
#include "jsversion.h"
#include "jsopcode.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "js/Vector.h"

#define JS_KEYWORD(keyword, type, op, version) \
    extern const char js_##keyword##_str[];
#include "jskeyword.tbl"
#undef JS_KEYWORD

namespace js {

enum TokenKind {
    TOK_ERROR = -1,                
    TOK_EOF,                       
    TOK_EOL,                       
    TOK_SEMI,                      
    TOK_COMMA,                     
    TOK_HOOK, TOK_COLON,           
    TOK_OR,                        
    TOK_AND,                       
    TOK_BITOR,                     
    TOK_BITXOR,                    
    TOK_BITAND,                    
    TOK_PLUS,                      
    TOK_MINUS,                     
    TOK_STAR,                      
    TOK_DIV,                       
    TOK_MOD,                       
    TOK_INC, TOK_DEC,              
    TOK_DOT,                       
    TOK_TRIPLEDOT,                 
    TOK_LB, TOK_RB,                
    TOK_LC, TOK_RC,                
    TOK_LP, TOK_RP,                
    TOK_NAME,                      
    TOK_NUMBER,                    
    TOK_STRING,                    
    TOK_REGEXP,                    
    TOK_TRUE,                      
    TOK_FALSE,                     
    TOK_NULL,                      
    TOK_THIS,                      
    TOK_FUNCTION,                  
    TOK_IF,                        
    TOK_ELSE,                      
    TOK_SWITCH,                    
    TOK_CASE,                      
    TOK_DEFAULT,                   
    TOK_WHILE,                     
    TOK_DO,                        
    TOK_FOR,                       
    TOK_BREAK,                     
    TOK_CONTINUE,                  
    TOK_IN,                        
    TOK_VAR,                       
    TOK_CONST,                     
    TOK_WITH,                      
    TOK_RETURN,                    
    TOK_NEW,                       
    TOK_DELETE,                    
    TOK_TRY,                       
    TOK_CATCH,                     
    TOK_FINALLY,                   
    TOK_THROW,                     
    TOK_INSTANCEOF,                
    TOK_DEBUGGER,                  
    TOK_XMLSTAGO,                  
    TOK_XMLETAGO,                  
    TOK_XMLPTAGC,                  
    TOK_XMLTAGC,                   
    TOK_XMLNAME,                   
    TOK_XMLATTR,                   
    TOK_XMLSPACE,                  
    TOK_XMLTEXT,                   
    TOK_XMLCOMMENT,                
    TOK_XMLCDATA,                  
    TOK_XMLPI,                     
    TOK_AT,                        
    TOK_DBLCOLON,                  
    TOK_DBLDOT,                    
    TOK_FILTER,                    
    TOK_XMLELEM,                   
    TOK_XMLLIST,                   
    TOK_YIELD,                     
    TOK_LEXICALSCOPE,              
    TOK_LET,                       
    TOK_RESERVED,                  
    TOK_STRICT_RESERVED,           

    




    
    TOK_STRICTEQ,
    TOK_EQUALITY_START = TOK_STRICTEQ,
    TOK_EQ,
    TOK_STRICTNE,
    TOK_NE,
    TOK_EQUALITY_LAST = TOK_NE,

    
    TOK_TYPEOF,
    TOK_VOID,
    TOK_NOT,
    TOK_BITNOT,

    
    TOK_LT,
    TOK_RELOP_START = TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_RELOP_LAST = TOK_GE,

    
    TOK_LSH,
    TOK_SHIFTOP_START = TOK_LSH,
    TOK_RSH,
    TOK_URSH,
    TOK_SHIFTOP_LAST = TOK_URSH,

    
    TOK_ASSIGN,                    
    TOK_ASSIGNMENT_START = TOK_ASSIGN,
    TOK_ADDASSIGN,
    TOK_SUBASSIGN,
    TOK_BITORASSIGN,
    TOK_BITXORASSIGN,
    TOK_BITANDASSIGN,
    TOK_LSHASSIGN,
    TOK_RSHASSIGN,
    TOK_URSHASSIGN,
    TOK_MULASSIGN,
    TOK_DIVASSIGN,
    TOK_MODASSIGN,
    TOK_ASSIGNMENT_LAST = TOK_MODASSIGN,

    TOK_LIMIT                      
};

inline bool
TokenKindIsEquality(TokenKind tt)
{
    return TOK_EQUALITY_START <= tt && tt <= TOK_EQUALITY_LAST;
}

inline bool
TokenKindIsRelational(TokenKind tt)
{
    return TOK_RELOP_START <= tt && tt <= TOK_RELOP_LAST;
}

inline bool
TokenKindIsShift(TokenKind tt)
{
    return TOK_SHIFTOP_START <= tt && tt <= TOK_SHIFTOP_LAST;
}

inline bool
TokenKindIsAssignment(TokenKind tt)
{
    return TOK_ASSIGNMENT_START <= tt && tt <= TOK_ASSIGNMENT_LAST;
}

inline bool
TokenKindIsDecl(TokenKind tt)
{
#if JS_HAS_BLOCK_SCOPE
    return tt == TOK_VAR || tt == TOK_LET;
#else
    return tt == TOK_VAR;
#endif
}

struct TokenPtr {
    uint32_t            index;          
    uint32_t            lineno;         

    bool operator==(const TokenPtr& bptr) const {
        return index == bptr.index && lineno == bptr.lineno;
    }

    bool operator!=(const TokenPtr& bptr) const {
        return index != bptr.index || lineno != bptr.lineno;
    }

    bool operator <(const TokenPtr& bptr) const {
        return lineno < bptr.lineno ||
               (lineno == bptr.lineno && index < bptr.index);
    }

    bool operator <=(const TokenPtr& bptr) const {
        return lineno < bptr.lineno ||
               (lineno == bptr.lineno && index <= bptr.index);
    }

    bool operator >(const TokenPtr& bptr) const {
        return !(*this <= bptr);
    }

    bool operator >=(const TokenPtr& bptr) const {
        return !(*this < bptr);
    }
};

struct TokenPos {
    TokenPtr          begin;          
    TokenPtr          end;            

    static TokenPos make(const TokenPtr &begin, const TokenPtr &end) {
        JS_ASSERT(begin <= end);
        TokenPos pos = {begin, end};
        return pos;
    }

    
    static TokenPos box(const TokenPos &left, const TokenPos &right) {
        JS_ASSERT(left.begin <= left.end);
        JS_ASSERT(left.end <= right.begin);
        JS_ASSERT(right.begin <= right.end);
        TokenPos pos = {left.begin, right.end};
        return pos;
    }

    bool operator==(const TokenPos& bpos) const {
        return begin == bpos.begin && end == bpos.end;
    }

    bool operator!=(const TokenPos& bpos) const {
        return begin != bpos.begin || end != bpos.end;
    }

    bool operator <(const TokenPos& bpos) const {
        return begin < bpos.begin;
    }

    bool operator <=(const TokenPos& bpos) const {
        return begin <= bpos.begin;
    }

    bool operator >(const TokenPos& bpos) const {
        return !(*this <= bpos);
    }

    bool operator >=(const TokenPos& bpos) const {
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
            union {
              private:
                friend struct Token;
                PropertyName *name;     
                JSAtom       *atom;     
            } n;
        } s;

      private:
        friend struct Token;
        struct {                        
            PropertyName *target;       
            JSAtom       *data;         
        } xmlpi;
        double          number;         
        RegExpFlag      reflags;        

    } u;

    

    




    void setName(JSOp op, PropertyName *name) {
        JS_ASSERT(op == JSOP_NAME);
        JS_ASSERT(!IsPoisonedPtr(name));
        u.s.op = op;
        u.s.n.name = name;
    }

    void setAtom(JSOp op, JSAtom *atom) {
        JS_ASSERT(op == JSOP_STRING || op == JSOP_XMLCOMMENT || JSOP_XMLCDATA);
        JS_ASSERT(!IsPoisonedPtr(atom));
        u.s.op = op;
        u.s.n.atom = atom;
    }

    void setProcessingInstruction(PropertyName *target, JSAtom *data) {
        JS_ASSERT(target);
        JS_ASSERT(data);
        JS_ASSERT(!target->empty());
        JS_ASSERT(!IsPoisonedPtr(target));
        JS_ASSERT(!IsPoisonedPtr(data));
        u.xmlpi.target = target;
        u.xmlpi.data = data;
    }

    void setRegExpFlags(js::RegExpFlag flags) {
        JS_ASSERT((flags & AllFlags) == flags);
        u.reflags = flags;
    }

    void setNumber(double n) {
        u.number = n;
    }

    

    PropertyName *name() const {
        JS_ASSERT(type == TOK_NAME);
        return u.s.n.name->asPropertyName(); 
    }

    JSAtom *atom() const {
        JS_ASSERT(type == TOK_STRING ||
                  type == TOK_XMLNAME ||
                  type == TOK_XMLATTR ||
                  type == TOK_XMLTEXT ||
                  type == TOK_XMLCDATA ||
                  type == TOK_XMLSPACE ||
                  type == TOK_XMLCOMMENT);
        return u.s.n.atom;
    }

    PropertyName *xmlPITarget() const {
        JS_ASSERT(type == TOK_XMLPI);
        return u.xmlpi.target;
    }
    JSAtom *xmlPIData() const {
        JS_ASSERT(type == TOK_XMLPI);
        return u.xmlpi.data;
    }

    js::RegExpFlag regExpFlags() const {
        JS_ASSERT(type == TOK_REGEXP);
        JS_ASSERT((u.reflags & AllFlags) == u.reflags);
        return u.reflags;
    }

    double number() const {
        JS_ASSERT(type == TOK_NUMBER);
        return u.number;
    }
};

#define t_op            u.s.op

enum TokenStreamFlags
{
    TSF_EOF = 0x02,             
    TSF_EOL = 0x04,             
    TSF_OPERAND = 0x08,         
    TSF_UNEXPECTED_EOF = 0x10,  
    TSF_KEYWORD_IS_NAME = 0x20, 
    TSF_DIRTYLINE = 0x40,       
    TSF_OWNFILENAME = 0x80,     
    TSF_XMLTAGMODE = 0x100,     
    TSF_XMLTEXTMODE = 0x200,    
    TSF_XMLONLYMODE = 0x400,    
    TSF_OCTAL_CHAR = 0x800,     

    


















    TSF_IN_HTML_COMMENT = 0x1000
};

struct Parser;









class StrictModeGetter {
    Parser *parser;
  public:
    StrictModeGetter(Parser *p) : parser(p) { }

    bool get() const;
};

class TokenStream
{
    
    enum {
        LINE_SEPARATOR = 0x2028,
        PARA_SEPARATOR = 0x2029
    };

    static const size_t ntokens = 4;                

    static const unsigned ntokensMask = ntokens - 1;

  public:
    typedef Vector<jschar, 32> CharBuffer;

    TokenStream(JSContext *cx, JSPrincipals *principals, JSPrincipals *originPrincipals,
                const jschar *base, size_t length, const char *filename, unsigned lineno,
                JSVersion version, StrictModeGetter *smg);

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
    unsigned getLineno() const { return lineno; }
    
    JSVersion versionNumber() const { return VersionNumber(version); }
    JSVersion versionWithFlags() const { return version; }
    bool allowsXML() const { return allowXML && !isStrictMode(); }
    bool hasMoarXML() const { return moarXML || VersionShouldParseXML(versionNumber()); }
    void setMoarXML(bool enabled) { moarXML = enabled; }

    bool isCurrentTokenEquality() const {
        return TokenKindIsEquality(currentToken().type);
    }

    bool isCurrentTokenRelational() const {
        return TokenKindIsRelational(currentToken().type);
    }

    bool isCurrentTokenShift() const {
        return TokenKindIsShift(currentToken().type);
    }

    bool isCurrentTokenAssignment() const {
        return TokenKindIsAssignment(currentToken().type);
    }

    
    void setXMLTagMode(bool enabled = true) { setFlag(enabled, TSF_XMLTAGMODE); }
    void setXMLOnlyMode(bool enabled = true) { setFlag(enabled, TSF_XMLONLYMODE); }
    void setUnexpectedEOF(bool enabled = true) { setFlag(enabled, TSF_UNEXPECTED_EOF); }
    void setOctalCharacterEscape(bool enabled = true) { setFlag(enabled, TSF_OCTAL_CHAR); }

    bool isStrictMode() const { return strictModeGetter ? strictModeGetter->get() : false; }
    bool isXMLTagMode() const { return !!(flags & TSF_XMLTAGMODE); }
    bool isXMLOnlyMode() const { return !!(flags & TSF_XMLONLYMODE); }
    bool isUnexpectedEOF() const { return !!(flags & TSF_UNEXPECTED_EOF); }
    bool isEOF() const { return !!(flags & TSF_EOF); }
    bool hasOctalCharacterEscape() const { return flags & TSF_OCTAL_CHAR; }

    
    
    bool reportCompileErrorNumberVA(ParseNode *pn, unsigned flags, unsigned errorNumber,
                                    va_list ap);

  private:
    static JSAtom *atomize(JSContext *cx, CharBuffer &cb);
    bool putIdentInTokenbuf(const jschar *identStart);

    



    class Flagger {
        TokenStream * const parent;
        unsigned       flags;
      public:
        Flagger(TokenStream *parent, unsigned withFlags) : parent(parent), flags(withFlags) {
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

    
    TokenKind getToken(unsigned withFlags) {
        Flagger flagger(this, withFlags);
        return getToken();
    }

    


    void ungetToken() {
        JS_ASSERT(lookahead < ntokensMask);
        lookahead++;
        cursor = (cursor - 1) & ntokensMask;
    }

    TokenKind peekToken() {
        if (lookahead != 0) {
            JS_ASSERT(lookahead == 1);
            return tokens[(cursor + lookahead) & ntokensMask].type;
        }
        TokenKind tt = getTokenInternal();
        ungetToken();
        return tt;
    }

    TokenKind peekToken(unsigned withFlags) {
        Flagger flagger(this, withFlags);
        return peekToken();
    }

    TokenKind peekTokenSameLine(unsigned withFlags = 0) {
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

    


    bool matchToken(TokenKind tt) {
        if (getToken() == tt)
            return true;
        ungetToken();
        return false;
    }

    bool matchToken(TokenKind tt, unsigned withFlags) {
        Flagger flagger(this, withFlags);
        return matchToken(tt);
    }

    void consumeKnownToken(TokenKind tt) {
        JS_ALWAYS_TRUE(matchToken(tt));
    }

    


    const jschar *releaseSourceMap() {
        const jschar* sm = sourceMap;
        sourceMap = NULL;
        return sm;
    }

    













    bool checkForKeyword(const jschar *s, size_t length, TokenKind *ttp, JSOp *topp);

  private:
    






    class TokenBuf {
      public:
        TokenBuf(const jschar *buf, size_t length)
          : base(buf), limit(buf + length), ptr(buf) { }

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
            ptr = NULL;
        }
#endif

        static bool isRawEOLChar(int32_t c) {
            return (c == '\n' || c == '\r' || c == LINE_SEPARATOR || c == PARA_SEPARATOR);
        }

        
        
        const jschar *findEOLMax(const jschar *p, size_t max);

      private:
        const jschar *base;             
        const jschar *limit;            
        const jschar *ptr;              
    };

    TokenKind getTokenInternal();     

    int32_t getChar();
    int32_t getCharIgnoreEOL();
    void ungetChar(int32_t c);
    void ungetCharIgnoreEOL(int32_t c);
    Token *newToken(ptrdiff_t adjust);
    bool peekUnicodeEscape(int32_t *c);
    bool matchUnicodeEscapeIdStart(int32_t *c);
    bool matchUnicodeEscapeIdent(int32_t *c);
    bool peekChars(int n, jschar *cp);
    bool getAtLine();
    bool getAtSourceMappingURL();

    bool getXMLEntity();
    bool getXMLTextOrTag(TokenKind *ttp, Token **tpp);
    bool getXMLMarkup(TokenKind *ttp, Token **tpp);

    bool matchChar(int32_t expect) {
        int32_t c = getChar();
        if (c == expect)
            return true;
        ungetChar(c);
        return false;
    }

    void consumeKnownChar(int32_t expect) {
        mozilla::DebugOnly<int32_t> c = getChar();
        JS_ASSERT(c == expect);
    }

    int32_t peekChar() {
        int32_t c = getChar();
        ungetChar(c);
        return c;
    }

    void skipChars(int n) {
        while (--n >= 0)
            getChar();
    }

    void updateLineInfoForEOL();
    void updateFlagsForEOL();

    Token               tokens[ntokens];
    JS::SkipRoot        tokensRoot;     
    unsigned            cursor;         
    unsigned            lookahead;      
    unsigned            lineno;         
    unsigned            flags;          
    const jschar        *linebase;      
    const jschar        *prevLinebase;  
    JS::SkipRoot        linebaseRoot;
    JS::SkipRoot        prevLinebaseRoot;
    TokenBuf            userbuf;        
    JS::SkipRoot        userbufRoot;
    const char          *filename;      
    jschar              *sourceMap;     
    void                *listenerTSData;
    CharBuffer          tokenbuf;       
    int8_t              oneCharTokens[128];  
    bool                maybeEOL[256];       
    bool                maybeStrSpecial[256];
    JSVersion           version;        
    bool                allowXML;       
    bool                moarXML;        
    JSContext           *const cx;
    JSPrincipals        *const originPrincipals;
    StrictModeGetter    *strictModeGetter; 
};

struct KeywordInfo {
    const char  *chars;         
    TokenKind   tokentype;
    JSOp        op;             
    JSVersion   version;        
};





const KeywordInfo *
FindKeyword(const jschar *s, size_t length);





JSBool
IsIdentifier(JSLinearString *str);





#define JSREPORT_UC 0x100






bool
ReportCompileErrorNumber(JSContext *cx, TokenStream *ts, ParseNode *pn, unsigned flags,
                         unsigned errorNumber, ...);





bool
ReportStrictModeError(JSContext *cx, TokenStream *ts, ParseNode *pn, unsigned errorNumber, ...);

} 

extern JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file);

#ifdef DEBUG
extern const char *
TokenKindToString(js::TokenKind tt);
#endif

#endif 
