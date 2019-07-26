





#ifndef frontend_TokenStream_h
#define frontend_TokenStream_h





#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "jscntxt.h"
#include "jsopcode.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"

#include "js/Vector.h"

namespace js {
namespace frontend {



enum TokenKind {
    TOK_ERROR = 0,                 
    TOK_EOF,                       
    TOK_EOL,                       
    TOK_SEMI,                      
    TOK_COMMA,                     
    TOK_HOOK, TOK_COLON,           
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
    TOK_DEBUGGER,                  
    TOK_YIELD,                     
    TOK_LET,                       
    TOK_EXPORT,                    
    TOK_IMPORT,                    
    TOK_RESERVED,                  
    TOK_STRICT_RESERVED,           

    




    



    TOK_OR,                        
    TOK_BINOP_FIRST = TOK_OR,
    TOK_AND,                       
    TOK_BITOR,                     
    TOK_BITXOR,                    
    TOK_BITAND,                    

    
    TOK_STRICTEQ,
    TOK_EQUALITY_START = TOK_STRICTEQ,
    TOK_EQ,
    TOK_STRICTNE,
    TOK_NE,
    TOK_EQUALITY_LAST = TOK_NE,

    
    TOK_LT,
    TOK_RELOP_START = TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_RELOP_LAST = TOK_GE,

    TOK_INSTANCEOF,                
    TOK_IN,                        

    
    TOK_LSH,
    TOK_SHIFTOP_START = TOK_LSH,
    TOK_RSH,
    TOK_URSH,
    TOK_SHIFTOP_LAST = TOK_URSH,

    TOK_PLUS,                      
    TOK_MINUS,                     
    TOK_STAR,                      
    TOK_DIV,                       
    TOK_MOD,                       
    TOK_BINOP_LAST = TOK_MOD,

    
    TOK_TYPEOF,
    TOK_VOID,
    TOK_NOT,
    TOK_BITNOT,

    TOK_ARROW,                     

    
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
TokenKindIsBinaryOp(TokenKind tt)
{
    return TOK_BINOP_FIRST <= tt && tt <= TOK_BINOP_LAST;
}

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

struct TokenPos {
    uint32_t          begin;          
    uint32_t          end;            

    TokenPos() {}
    TokenPos(uint32_t begin, uint32_t end) : begin(begin), end(end) {}

    
    static TokenPos box(const TokenPos &left, const TokenPos &right) {
        JS_ASSERT(left.begin <= left.end);
        JS_ASSERT(left.end <= right.begin);
        JS_ASSERT(right.begin <= right.end);
        return TokenPos(left.begin, right.end);
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

    bool encloses(const TokenPos& pos) const {
        return begin <= pos.begin && pos.end <= end;
    }
};

enum DecimalPoint { NoDecimal = false, HasDecimal = true };

struct Token {
    TokenKind           type;           
    TokenPos            pos;            
    union {
      private:
        friend struct Token;
        PropertyName *name;             
        JSAtom       *atom;             
        struct {
            double       value;         
            DecimalPoint decimalPoint;  
        } number;
        RegExpFlag      reflags;        

    } u;

    

    




    void setName(PropertyName *name) {
        JS_ASSERT(!IsPoisonedPtr(name));
        u.name = name;
    }

    void setAtom(JSAtom *atom) {
        JS_ASSERT(!IsPoisonedPtr(atom));
        u.atom = atom;
    }

    void setRegExpFlags(js::RegExpFlag flags) {
        JS_ASSERT((flags & AllFlags) == flags);
        u.reflags = flags;
    }

    void setNumber(double n, DecimalPoint decimalPoint) {
        u.number.value = n;
        u.number.decimalPoint = decimalPoint;
    }

    

    PropertyName *name() const {
        JS_ASSERT(type == TOK_NAME);
        return u.name->asPropertyName(); 
    }

    JSAtom *atom() const {
        JS_ASSERT(type == TOK_STRING);
        return u.atom;
    }

    js::RegExpFlag regExpFlags() const {
        JS_ASSERT(type == TOK_REGEXP);
        JS_ASSERT((u.reflags & AllFlags) == u.reflags);
        return u.reflags;
    }

    double number() const {
        JS_ASSERT(type == TOK_NUMBER);
        return u.number.value;
    }

    DecimalPoint decimalPoint() const {
        JS_ASSERT(type == TOK_NUMBER);
        return u.number.decimalPoint;
    }
};

enum TokenStreamFlags
{
    TSF_EOF = 0x02,             
    TSF_EOL = 0x04,             
    TSF_OPERAND = 0x08,         
    TSF_UNEXPECTED_EOF = 0x10,  
    TSF_KEYWORD_IS_NAME = 0x20, 
    TSF_DIRTYLINE = 0x40,       
    TSF_OCTAL_CHAR = 0x80,      
    TSF_HAD_ERROR = 0x100,      

    


















    TSF_IN_HTML_COMMENT = 0x200
};

struct CompileError {
    JSContext *cx;
    JSErrorReport report;
    char *message;
    ErrorArgumentsType argumentsType;
    CompileError(JSContext *cx)
      : cx(cx), message(NULL), argumentsType(ArgumentsAreUnicode)
    {
        mozilla::PodZero(&report);
    }
    ~CompileError();
    void throwError();
};








class StrictModeGetter {
  public:
    virtual bool strictMode() = 0;
};









































class MOZ_STACK_CLASS TokenStream
{
    
    enum {
        LINE_SEPARATOR = 0x2028,
        PARA_SEPARATOR = 0x2029
    };

    static const size_t ntokens = 4;                

    static const unsigned maxLookahead = 2;
    static const unsigned ntokensMask = ntokens - 1;

  public:
    typedef Vector<jschar, 32> CharBuffer;

    TokenStream(ExclusiveContext *cx, const CompileOptions &options,
                const jschar *base, size_t length, StrictModeGetter *smg);

    ~TokenStream();

    
    bool onCurrentLine(const TokenPos &pos) const { return srcCoords.isOnThisLine(pos.end, lineno); }
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
    unsigned getColumn() const { return userbuf.addressOfNextRawChar() - linebase - 1; }
    JSPrincipals *getOriginPrincipals() const { return originPrincipals; }
    JSVersion versionNumber() const { return VersionNumber(options().version); }
    JSVersion versionWithFlags() const { return options().version; }
    bool hadError() const { return !!(flags & TSF_HAD_ERROR); }

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

    
    void setUnexpectedEOF(bool enabled = true) { setFlag(enabled, TSF_UNEXPECTED_EOF); }

    bool isUnexpectedEOF() const { return !!(flags & TSF_UNEXPECTED_EOF); }
    bool isEOF() const { return !!(flags & TSF_EOF); }
    bool sawOctalEscape() const { return !!(flags & TSF_OCTAL_CHAR); }

    
    bool reportError(unsigned errorNumber, ...);
    bool reportWarning(unsigned errorNumber, ...);

    
    
    
    bool reportCompileErrorNumberVA(uint32_t offset, unsigned flags, unsigned errorNumber,
                                    va_list args);
    bool reportStrictModeErrorNumberVA(uint32_t offset, bool strictMode, unsigned errorNumber,
                                       va_list args);
    bool reportStrictWarningErrorNumberVA(uint32_t offset, unsigned errorNumber,
                                          va_list args);

    
    void reportAsmJSError(uint32_t offset, unsigned errorNumber, ...);

  private:
    
    
    bool reportStrictModeError(unsigned errorNumber, ...);
    bool strictMode() const { return strictModeGetter && strictModeGetter->strictMode(); }

    void onError();
    static JSAtom *atomize(ExclusiveContext *cx, CharBuffer &cb);
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
        JS_ASSERT(lookahead < maxLookahead);
        lookahead++;
        cursor = (cursor - 1) & ntokensMask;
    }

    TokenKind peekToken() {
        if (lookahead != 0)
            return tokens[(cursor + 1) & ntokensMask].type;
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

        if (lookahead != 0)
            return tokens[(cursor + 1) & ntokensMask].type;

        



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

    bool matchContextualKeyword(Handle<PropertyName*> keyword) {
        if (getToken() == TOK_NAME && currentToken().name() == keyword)
            return true;
        ungetToken();
        return false;
    }

    bool nextTokenEndsExpr() {
        return isExprEnding[peekToken()];
    }

    class MOZ_STACK_CLASS Position {
      public:
        








        Position(AutoKeepAtoms&) { }
      private:
        Position(const Position&) MOZ_DELETE;
        friend class TokenStream;
        const jschar *buf;
        unsigned flags;
        unsigned lineno;
        const jschar *linebase;
        const jschar *prevLinebase;
        Token currentToken;
        unsigned lookahead;
        Token lookaheadTokens[maxLookahead];
    };

    void advance(size_t position);
    void tell(Position *);
    void seek(const Position &pos);
    void seek(const Position &pos, const TokenStream &other);

    size_t positionToOffset(const Position &pos) const {
        return pos.buf - userbuf.base();
    }

    bool hasSourceMap() const {
        return sourceMap != NULL;
    }

    


    jschar *releaseSourceMap() {
        JS_ASSERT(hasSourceMap());
        jschar *sm = sourceMap;
        sourceMap = NULL;
        return sm;
    }

    











    bool checkForKeyword(const jschar *s, size_t length, TokenKind *ttp);

    
    
    class SourceCoords
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        Vector<uint32_t, 128> lineStartOffsets_;
        uint32_t            initialLineNum_;

        
        
        mutable uint32_t    lastLineIndex_;

        uint32_t lineIndexOf(uint32_t offset) const;

        static const uint32_t MAX_PTR = UINT32_MAX;

        uint32_t lineIndexToNum(uint32_t lineIndex) const { return lineIndex + initialLineNum_; }
        uint32_t lineNumToIndex(uint32_t lineNum)   const { return lineNum   - initialLineNum_; }

      public:
        SourceCoords(ExclusiveContext *cx, uint32_t ln);

        void add(uint32_t lineNum, uint32_t lineStartOffset);
        void fill(const SourceCoords &other);

        bool isOnThisLine(uint32_t offset, uint32_t lineNum) const {
            uint32_t lineIndex = lineNumToIndex(lineNum);
            JS_ASSERT(lineIndex + 1 < lineStartOffsets_.length());  
            return lineStartOffsets_[lineIndex] <= offset &&
                   offset < lineStartOffsets_[lineIndex + 1];
        }

        uint32_t lineNum(uint32_t offset) const;
        uint32_t columnIndex(uint32_t offset) const;
        void lineNumAndColumnIndex(uint32_t offset, uint32_t *lineNum, uint32_t *columnIndex) const;
    };

    SourceCoords srcCoords;

    JSAtomState &names() const {
        return cx->names();
    }

    ExclusiveContext *context() const {
        return cx;
    }

    const CompileOptions &options() const {
        return options_;
    }

  private:
    






    class TokenBuf {
      public:
        TokenBuf(ExclusiveContext *cx, const jschar *buf, size_t length)
          : base_(buf), limit_(buf + length), ptr(buf),
            skipBase(cx, &base_), skipLimit(cx, &limit_), skipPtr(cx, &ptr)
        { }

        bool hasRawChars() const {
            return ptr < limit_;
        }

        bool atStart() const {
            return ptr == base_;
        }

        const jschar *base() const {
            return base_;
        }

        const jschar *limit() const {
            return limit_;
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

        const jschar *addressOfNextRawChar(bool allowPoisoned = false) const {
            JS_ASSERT_IF(!allowPoisoned, ptr);     
            return ptr;
        }

        
        void setAddressOfNextRawChar(const jschar *a, bool allowPoisoned = false) {
            JS_ASSERT_IF(!allowPoisoned, a);
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
        const jschar *base_;            
        const jschar *limit_;           
        const jschar *ptr;              

        
        SkipRoot skipBase, skipLimit, skipPtr;
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
    bool getSourceMappingURL(bool isMultiline, bool shouldWarnDeprecated);

    
    bool matchChar(int32_t expect) {
        MOZ_ASSERT(!TokenBuf::isRawEOLChar(expect));
        return JS_LIKELY(userbuf.hasRawChars()) &&
               userbuf.matchRawChar(expect);
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

    
    const CompileOptions &options_;

    Token               tokens[ntokens];
    unsigned            cursor;         
    unsigned            lookahead;      
    unsigned            lineno;         
    unsigned            flags;          
    const jschar        *linebase;      
    const jschar        *prevLinebase;  
    TokenBuf            userbuf;        
    const char          *filename;      
    jschar              *sourceMap;     
    CharBuffer          tokenbuf;       
    bool                maybeEOL[256];       
    bool                maybeStrSpecial[256];
    uint8_t             isExprEnding[TOK_LIMIT]; 
    ExclusiveContext    *const cx;
    JSPrincipals        *const originPrincipals;
    StrictModeGetter    *strictModeGetter; 

    




    SkipRoot            tokenSkip;

    
    SkipRoot            linebaseSkip;
    SkipRoot            prevLinebaseSkip;
};





#define JSREPORT_UC 0x100

} 
} 

extern JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file);

#ifdef DEBUG
extern const char *
TokenKindToString(js::frontend::TokenKind tt);
#endif

#endif 
