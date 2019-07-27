





#ifndef frontend_TokenStream_h
#define frontend_TokenStream_h



#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "jscntxt.h"
#include "jspubtd.h"

#include "frontend/TokenKind.h"
#include "js/Vector.h"
#include "vm/RegExpObject.h"

struct KeywordInfo;

namespace js {
namespace frontend {

struct TokenPos {
    uint32_t    begin;  
    uint32_t    end;    

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

struct Token
{
    TokenKind           type;           
    TokenPos            pos;            
    union {
      private:
        friend struct Token;
        PropertyName    *name;          
        JSAtom          *atom;          
        struct {
            double      value;          
            DecimalPoint decimalPoint;  
        } number;
        RegExpFlag      reflags;        
                                        
    } u;

    
    
    
    
    
    
    
    
    
    
    
    
    Token()
      : type(TOK_ERROR),
        pos(0, 0)
    {
    }

    

    void setName(PropertyName *name) {
        JS_ASSERT(type == TOK_NAME);
        JS_ASSERT(!IsPoisonedPtr(name));
        u.name = name;
    }

    void setAtom(JSAtom *atom) {
        JS_ASSERT (type == TOK_STRING ||
                   type == TOK_TEMPLATE_HEAD ||
                   type == TOK_NO_SUBS_TEMPLATE);
        JS_ASSERT(!IsPoisonedPtr(atom));
        u.atom = atom;
    }

    void setRegExpFlags(js::RegExpFlag flags) {
        JS_ASSERT(type == TOK_REGEXP);
        JS_ASSERT((flags & AllFlags) == flags);
        u.reflags = flags;
    }

    void setNumber(double n, DecimalPoint decimalPoint) {
        JS_ASSERT(type == TOK_NUMBER);
        u.number.value = n;
        u.number.decimalPoint = decimalPoint;
    }

    

    PropertyName *name() const {
        JS_ASSERT(type == TOK_NAME);
        return u.name->asPropertyName(); 
    }

    JSAtom *atom() const {
        JS_ASSERT (type == TOK_STRING ||
                   type == TOK_TEMPLATE_HEAD ||
                   type == TOK_NO_SUBS_TEMPLATE);
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

struct CompileError {
    JSErrorReport report;
    char *message;
    ErrorArgumentsType argumentsType;
    CompileError()
      : message(nullptr), argumentsType(ArgumentsAreUnicode)
    {
        mozilla::PodZero(&report);
    }
    ~CompileError();
    void throwError(JSContext *cx);

  private:
    
    
    void operator=(const CompileError &) MOZ_DELETE;
    CompileError(const CompileError &) MOZ_DELETE;
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

    TokenStream(ExclusiveContext *cx, const ReadOnlyCompileOptions &options,
                const jschar *base, size_t length, StrictModeGetter *smg);

    ~TokenStream();

    
    const Token &currentToken() const { return tokens[cursor]; }
    bool isCurrentTokenType(TokenKind type) const {
        return currentToken().type == type;
    }
    const CharBuffer &getTokenbuf() const { return tokenbuf; }
    const char *getFilename() const { return filename; }
    unsigned getLineno() const { return lineno; }
    unsigned getColumn() const { return userbuf.addressOfNextRawChar() - linebase - 1; }
    JSPrincipals *getOriginPrincipals() const { return originPrincipals; }
    JSVersion versionNumber() const { return VersionNumber(options().version); }
    JSVersion versionWithFlags() const { return options().version; }

    PropertyName *currentName() const {
        if (isCurrentTokenType(TOK_YIELD))
            return cx->names().yield;
        JS_ASSERT(isCurrentTokenType(TOK_NAME));
        return currentToken().name();
    }

    bool isCurrentTokenAssignment() const {
        return TokenKindIsAssignment(currentToken().type);
    }

    
    bool isEOF() const { return flags.isEOF; }
    bool sawOctalEscape() const { return flags.sawOctalEscape; }
    bool hadError() const { return flags.hadError; }

    
    bool reportError(unsigned errorNumber, ...);
    bool reportWarning(unsigned errorNumber, ...);

    static const uint32_t NoOffset = UINT32_MAX;

    
    
    
    bool reportCompileErrorNumberVA(uint32_t offset, unsigned flags, unsigned errorNumber,
                                    va_list args);
    bool reportStrictModeErrorNumberVA(uint32_t offset, bool strictMode, unsigned errorNumber,
                                       va_list args);
    bool reportStrictWarningErrorNumberVA(uint32_t offset, unsigned errorNumber,
                                          va_list args);

    
    void reportAsmJSError(uint32_t offset, unsigned errorNumber, ...);

    JSAtom *getRawTemplateStringAtom() {
        JS_ASSERT(currentToken().type == TOK_TEMPLATE_HEAD ||
                  currentToken().type == TOK_NO_SUBS_TEMPLATE);
        const jschar *cur = userbuf.base() + currentToken().pos.begin + 1;
        const jschar *end;
        if (currentToken().type == TOK_TEMPLATE_HEAD) {
            
            end = userbuf.base() + currentToken().pos.end - 2;
        } else {
            
            end = userbuf.base() + currentToken().pos.end - 1;
        }

        CharBuffer charbuf(cx);
        while (cur < end) {
            int32_t ch = *cur;
            if (ch == '\r') {
                ch = '\n';
                if ((cur + 1 < end) && (*(cur + 1) == '\n'))
                    cur++;
            }
            if (!charbuf.append(ch))
                return nullptr;
            cur++;
        }
        return AtomizeChars(cx, charbuf.begin(), charbuf.length());
    }

  private:
    
    
    bool reportStrictModeError(unsigned errorNumber, ...);
    bool strictMode() const { return strictModeGetter && strictModeGetter->strictMode(); }

    void onError();
    static JSAtom *atomize(ExclusiveContext *cx, CharBuffer &cb);
    bool putIdentInTokenbuf(const jschar *identStart);

    struct Flags
    {
        bool isEOF:1;           
        bool isDirtyLine:1;     
        bool sawOctalEscape:1;  
        bool hadError:1;        

        Flags()
          : isEOF(), isDirtyLine(), sawOctalEscape(), hadError()
        {}
    };

  public:
    
    enum Modifier
    {
        None,           
        Operand,        
                        
                        
                        
        KeywordIsName,  
        TemplateTail,   
    };

    
    
    TokenKind getToken(Modifier modifier = None) {
        
        if (lookahead != 0) {
            lookahead--;
            cursor = (cursor + 1) & ntokensMask;
            TokenKind tt = currentToken().type;
            JS_ASSERT(tt != TOK_EOL);
            return tt;
        }

        return getTokenInternal(modifier);
    }

    
    void ungetToken() {
        JS_ASSERT(lookahead < maxLookahead);
        lookahead++;
        cursor = (cursor - 1) & ntokensMask;
    }

    TokenKind peekToken(Modifier modifier = None) {
        if (lookahead != 0)
            return tokens[(cursor + 1) & ntokensMask].type;
        TokenKind tt = getTokenInternal(modifier);
        ungetToken();
        return tt;
    }

    TokenPos peekTokenPos(Modifier modifier = None) {
        if (lookahead != 0)
            return tokens[(cursor + 1) & ntokensMask].pos;
        getTokenInternal(modifier);
        ungetToken();
        JS_ASSERT(lookahead != 0);
        return tokens[(cursor + 1) & ntokensMask].pos;
    }

    
    
    
    
    
    MOZ_ALWAYS_INLINE TokenKind peekTokenSameLine(Modifier modifier = None) {
       const Token &curr = currentToken();

        
        
        
        
        
        if (lookahead != 0 && srcCoords.isOnThisLine(curr.pos.end, lineno))
            return tokens[(cursor + 1) & ntokensMask].type;

        
        
        
        
        
        
        
        (void)getToken(modifier);
        const Token &next = currentToken();
        ungetToken();
        return srcCoords.lineNum(curr.pos.end) == srcCoords.lineNum(next.pos.begin)
               ? next.type
               : TOK_EOL;
    }

    
    bool matchToken(TokenKind tt, Modifier modifier = None) {
        if (getToken(modifier) == tt)
            return true;
        ungetToken();
        return false;
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
        
        
        
        
        
        
        
        explicit Position(AutoKeepAtoms&) { }
      private:
        Position(const Position&) MOZ_DELETE;
        friend class TokenStream;
        const jschar *buf;
        Flags flags;
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
    bool seek(const Position &pos, const TokenStream &other);

    size_t positionToOffset(const Position &pos) const {
        return pos.buf - userbuf.base();
    }

    const jschar *rawBase() const {
        return userbuf.base();
    }

    const jschar *rawLimit() const {
        return userbuf.limit();
    }

    bool hasDisplayURL() const {
        return displayURL_ != nullptr;
    }

    jschar *displayURL() {
        return displayURL_.get();
    }

    bool hasSourceMapURL() const {
        return sourceMapURL_ != nullptr;
    }

    jschar *sourceMapURL() {
        return sourceMapURL_.get();
    }

    
    
    
    
    
    
    
    
    
    
    bool checkForKeyword(const KeywordInfo *kw, TokenKind *ttp);
    bool checkForKeyword(const jschar *s, size_t length, TokenKind *ttp);
    bool checkForKeyword(JSAtom *atom, TokenKind *ttp);

    
    
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
        bool fill(const SourceCoords &other);

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

    const ReadOnlyCompileOptions &options() const {
        return options_;
    }

  private:
    
    
    
    
    
    class TokenBuf {
      public:
        TokenBuf(ExclusiveContext *cx, const jschar *buf, size_t length)
          : base_(buf), limit_(buf + length), ptr(buf)
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
            ptr = nullptr;
        }
#endif

        static bool isRawEOLChar(int32_t c) {
            return c == '\n' || c == '\r' || c == LINE_SEPARATOR || c == PARA_SEPARATOR;
        }

        
        
        const jschar *findEOLMax(const jschar *p, size_t max);

      private:
        const jschar *base_;            
        const jschar *limit_;           
        const jschar *ptr;              
    };

    TokenKind getTokenInternal(Modifier modifier);

    bool getStringOrTemplateToken(int qc, Token **tp);

    int32_t getChar();
    int32_t getCharIgnoreEOL();
    void ungetChar(int32_t c);
    void ungetCharIgnoreEOL(int32_t c);
    Token *newToken(ptrdiff_t adjust);
    bool peekUnicodeEscape(int32_t *c);
    bool matchUnicodeEscapeIdStart(int32_t *c);
    bool matchUnicodeEscapeIdent(int32_t *c);
    bool peekChars(int n, jschar *cp);

    bool getDirectives(bool isMultiline, bool shouldWarnDeprecated);
    bool getDirective(bool isMultiline, bool shouldWarnDeprecated,
                      const char *directive, int directiveLength,
                      const char *errorMsgPragma,
                      mozilla::UniquePtr<jschar[], JS::FreePolicy> *destination);
    bool getDisplayURL(bool isMultiline, bool shouldWarnDeprecated);
    bool getSourceMappingURL(bool isMultiline, bool shouldWarnDeprecated);

    
    bool matchChar(int32_t expect) {
        MOZ_ASSERT(!TokenBuf::isRawEOLChar(expect));
        return MOZ_LIKELY(userbuf.hasRawChars()) &&
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

    
    const ReadOnlyCompileOptions &options_;

    Token               tokens[ntokens];    
    unsigned            cursor;             
    unsigned            lookahead;          
    unsigned            lineno;             
    Flags               flags;              
    const jschar        *linebase;          
    const jschar        *prevLinebase;      
    TokenBuf            userbuf;            
    const char          *filename;          
    mozilla::UniquePtr<jschar[], JS::FreePolicy> displayURL_; 
    mozilla::UniquePtr<jschar[], JS::FreePolicy> sourceMapURL_; 
    CharBuffer          tokenbuf;           
    bool                maybeEOL[256];      
    bool                maybeStrSpecial[256];   
    uint8_t             isExprEnding[TOK_LIMIT];
    ExclusiveContext    *const cx;
    JSPrincipals        *const originPrincipals;
    StrictModeGetter    *strictModeGetter;  
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
