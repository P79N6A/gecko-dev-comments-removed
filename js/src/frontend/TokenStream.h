





#ifndef frontend_TokenStream_h
#define frontend_TokenStream_h



#include "mozilla/ArrayUtils.h"
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

    
    static TokenPos box(const TokenPos& left, const TokenPos& right) {
        MOZ_ASSERT(left.begin <= left.end);
        MOZ_ASSERT(left.end <= right.begin);
        MOZ_ASSERT(right.begin <= right.end);
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
        PropertyName*   name;          
        JSAtom*         atom;          
        struct {
            double      value;          
            DecimalPoint decimalPoint;  
        } number;
        RegExpFlag      reflags;        
                                        
    } u;

    
    
    
    
    
    
    
    
    
    
    
    
    Token()
      : pos(0, 0)
    {
        MOZ_MAKE_MEM_UNDEFINED(&type, sizeof(type));
    }

    

    void setName(PropertyName* name) {
        MOZ_ASSERT(type == TOK_NAME);
        u.name = name;
    }

    void setAtom(JSAtom* atom) {
        MOZ_ASSERT(type == TOK_STRING ||
                   type == TOK_TEMPLATE_HEAD ||
                   type == TOK_NO_SUBS_TEMPLATE);
        u.atom = atom;
    }

    void setRegExpFlags(js::RegExpFlag flags) {
        MOZ_ASSERT(type == TOK_REGEXP);
        MOZ_ASSERT((flags & AllFlags) == flags);
        u.reflags = flags;
    }

    void setNumber(double n, DecimalPoint decimalPoint) {
        MOZ_ASSERT(type == TOK_NUMBER);
        u.number.value = n;
        u.number.decimalPoint = decimalPoint;
    }

    

    PropertyName* name() const {
        MOZ_ASSERT(type == TOK_NAME);
        return u.name->asPropertyName(); 
    }

    JSAtom* atom() const {
        MOZ_ASSERT(type == TOK_STRING ||
                   type == TOK_TEMPLATE_HEAD ||
                   type == TOK_NO_SUBS_TEMPLATE);
        return u.atom;
    }

    js::RegExpFlag regExpFlags() const {
        MOZ_ASSERT(type == TOK_REGEXP);
        MOZ_ASSERT((u.reflags & AllFlags) == u.reflags);
        return u.reflags;
    }

    double number() const {
        MOZ_ASSERT(type == TOK_NUMBER);
        return u.number.value;
    }

    DecimalPoint decimalPoint() const {
        MOZ_ASSERT(type == TOK_NUMBER);
        return u.number.decimalPoint;
    }
};

struct CompileError {
    JSErrorReport report;
    char* message;
    ErrorArgumentsType argumentsType;
    CompileError() : message(nullptr), argumentsType(ArgumentsAreUnicode) {}
    ~CompileError();
    void throwError(JSContext* cx);

  private:
    
    
    void operator=(const CompileError&) = delete;
    CompileError(const CompileError&) = delete;
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
    typedef Vector<char16_t, 32> CharBuffer;

    TokenStream(ExclusiveContext* cx, const ReadOnlyCompileOptions& options,
                const char16_t* base, size_t length, StrictModeGetter* smg);

    ~TokenStream();

    bool checkOptions();

    
    const Token& currentToken() const { return tokens[cursor]; }
    bool isCurrentTokenType(TokenKind type) const {
        return currentToken().type == type;
    }
    const CharBuffer& getTokenbuf() const { return tokenbuf; }
    const char* getFilename() const { return filename; }
    unsigned getLineno() const { return lineno; }
    unsigned getColumn() const { return userbuf.offset() - linebase - 1; }
    bool getMutedErrors() const { return mutedErrors; }
    JSVersion versionNumber() const { return VersionNumber(options().version); }
    JSVersion versionWithFlags() const { return options().version; }

    PropertyName* currentName() const {
        if (isCurrentTokenType(TOK_YIELD))
            return cx->names().yield;
        MOZ_ASSERT(isCurrentTokenType(TOK_NAME));
        return currentToken().name();
    }

    bool isCurrentTokenAssignment() const {
        return TokenKindIsAssignment(currentToken().type);
    }

    
    bool isEOF() const { return flags.isEOF; }
    bool sawOctalEscape() const { return flags.sawOctalEscape; }
    bool hadError() const { return flags.hadError; }

    
    bool reportError(unsigned errorNumber, ...);
    bool reportErrorNoOffset(unsigned errorNumber, ...);
    bool reportWarning(unsigned errorNumber, ...);

    static const uint32_t NoOffset = UINT32_MAX;

    
    
    
    bool reportCompileErrorNumberVA(uint32_t offset, unsigned flags, unsigned errorNumber,
                                    va_list args);
    bool reportStrictModeErrorNumberVA(uint32_t offset, bool strictMode, unsigned errorNumber,
                                       va_list args);
    bool reportStrictWarningErrorNumberVA(uint32_t offset, unsigned errorNumber,
                                          va_list args);

    
    void reportAsmJSError(uint32_t offset, unsigned errorNumber, ...);

    JSAtom* getRawTemplateStringAtom() {
        MOZ_ASSERT(currentToken().type == TOK_TEMPLATE_HEAD ||
                   currentToken().type == TOK_NO_SUBS_TEMPLATE);
        const char16_t* cur = userbuf.rawCharPtrAt(currentToken().pos.begin + 1);
        const char16_t* end;
        if (currentToken().type == TOK_TEMPLATE_HEAD) {
            
            end = userbuf.rawCharPtrAt(currentToken().pos.end - 2);
        } else {
            
            end = userbuf.rawCharPtrAt(currentToken().pos.end - 1);
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

    static JSAtom* atomize(ExclusiveContext* cx, CharBuffer& cb);
    bool putIdentInTokenbuf(const char16_t* identStart);

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

    
    
    bool getToken(TokenKind* ttp, Modifier modifier = None) {
        
        if (lookahead != 0) {
            MOZ_ASSERT(!flags.hadError);
            lookahead--;
            cursor = (cursor + 1) & ntokensMask;
            TokenKind tt = currentToken().type;
            MOZ_ASSERT(tt != TOK_EOL);
            *ttp = tt;
            return true;
        }

        return getTokenInternal(ttp, modifier);
    }

    
    void ungetToken() {
        MOZ_ASSERT(lookahead < maxLookahead);
        lookahead++;
        cursor = (cursor - 1) & ntokensMask;
    }

    bool peekToken(TokenKind* ttp, Modifier modifier = None) {
        if (lookahead > 0) {
            MOZ_ASSERT(!flags.hadError);
            *ttp = tokens[(cursor + 1) & ntokensMask].type;
            return true;
        }
        if (!getTokenInternal(ttp, modifier))
            return false;
        ungetToken();
        return true;
    }

    bool peekTokenPos(TokenPos* posp, Modifier modifier = None) {
        if (lookahead == 0) {
            TokenKind tt;
            if (!getTokenInternal(&tt, modifier))
                return false;
            ungetToken();
            MOZ_ASSERT(lookahead != 0);
        } else {
            MOZ_ASSERT(!flags.hadError);
        }
        *posp = tokens[(cursor + 1) & ntokensMask].pos;
        return true;
    }

    
    
    
    
    
    
    MOZ_ALWAYS_INLINE bool
    peekTokenSameLine(TokenKind* ttp, Modifier modifier = None) {
        const Token& curr = currentToken();

        
        
        
        
        
        if (lookahead != 0 && srcCoords.isOnThisLine(curr.pos.end, lineno)) {
            MOZ_ASSERT(!flags.hadError);
            *ttp = tokens[(cursor + 1) & ntokensMask].type;
            return true;
        }

        
        
        
        
        
        
        
        TokenKind tmp;
        if (!getToken(&tmp, modifier))
            return false;
        const Token& next = currentToken();
        ungetToken();

        *ttp = srcCoords.lineNum(curr.pos.end) == srcCoords.lineNum(next.pos.begin)
             ? next.type
             : TOK_EOL;
        return true;
    }

    
    bool matchToken(bool* matchedp, TokenKind tt, Modifier modifier = None) {
        TokenKind token;
        if (!getToken(&token, modifier))
            return false;
        if (token == tt) {
            *matchedp = true;
        } else {
            ungetToken();
            *matchedp = false;
        }
        return true;
    }

    void consumeKnownToken(TokenKind tt) {
        bool matched;
        MOZ_ASSERT(lookahead != 0);
        MOZ_ALWAYS_TRUE(matchToken(&matched, tt));
        MOZ_ALWAYS_TRUE(matched);
    }

    bool matchContextualKeyword(bool* matchedp, Handle<PropertyName*> keyword) {
        TokenKind token;
        if (!getToken(&token))
            return false;
        if (token == TOK_NAME && currentToken().name() == keyword) {
            *matchedp = true;
        } else {
            *matchedp = false;
            ungetToken();
        }
        return true;
    }

    bool nextTokenEndsExpr(bool* endsExpr) {
        TokenKind tt;
        if (!peekToken(&tt))
            return false;
        *endsExpr = isExprEnding[tt];
        return true;
    }

    class MOZ_STACK_CLASS Position {
      public:
        
        
        
        
        
        
        
        explicit Position(AutoKeepAtoms&) { }
      private:
        Position(const Position&) = delete;
        friend class TokenStream;
        const char16_t* buf;
        Flags flags;
        unsigned lineno;
        size_t linebase;
        size_t prevLinebase;
        Token currentToken;
        unsigned lookahead;
        Token lookaheadTokens[maxLookahead];
    };

    void advance(size_t position);
    void tell(Position*);
    void seek(const Position& pos);
    bool seek(const Position& pos, const TokenStream& other);
#ifdef DEBUG
    inline bool debugHasNoLookahead() const {
        return lookahead == 0;
    }
#endif

    const char16_t* rawCharPtrAt(size_t offset) const {
        return userbuf.rawCharPtrAt(offset);
    }

    const char16_t* rawLimit() const {
        return userbuf.limit();
    }

    bool hasDisplayURL() const {
        return displayURL_ != nullptr;
    }

    char16_t* displayURL() {
        return displayURL_.get();
    }

    bool hasSourceMapURL() const {
        return sourceMapURL_ != nullptr;
    }

    char16_t* sourceMapURL() {
        return sourceMapURL_.get();
    }

    
    
    
    
    
    
    
    
    
    
    bool checkForKeyword(const KeywordInfo* kw, TokenKind* ttp);
    bool checkForKeyword(const char16_t* s, size_t length, TokenKind* ttp);
    bool checkForKeyword(JSAtom* atom, TokenKind* ttp);

    
    
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
        SourceCoords(ExclusiveContext* cx, uint32_t ln);

        void add(uint32_t lineNum, uint32_t lineStartOffset);
        bool fill(const SourceCoords& other);

        bool isOnThisLine(uint32_t offset, uint32_t lineNum) const {
            uint32_t lineIndex = lineNumToIndex(lineNum);
            MOZ_ASSERT(lineIndex + 1 < lineStartOffsets_.length());  
            return lineStartOffsets_[lineIndex] <= offset &&
                   offset < lineStartOffsets_[lineIndex + 1];
        }

        uint32_t lineNum(uint32_t offset) const;
        uint32_t columnIndex(uint32_t offset) const;
        void lineNumAndColumnIndex(uint32_t offset, uint32_t* lineNum, uint32_t* columnIndex) const;
    };

    SourceCoords srcCoords;

    JSAtomState& names() const {
        return cx->names();
    }

    ExclusiveContext* context() const {
        return cx;
    }

    const ReadOnlyCompileOptions& options() const {
        return options_;
    }

  private:
    
    
    
    
    
    
    
    
    
    
    class TokenBuf {
      public:
        TokenBuf(ExclusiveContext* cx, const char16_t* buf, size_t length, size_t startOffset)
          : base_(buf),
            startOffset_(startOffset),
            limit_(buf + length),
            ptr(buf)
        { }

        bool hasRawChars() const {
            return ptr < limit_;
        }

        bool atStart() const {
            return offset() == 0;
        }

        size_t startOffset() const {
            return startOffset_;
        }

        size_t offset() const {
            return startOffset_ + mozilla::PointerRangeSize(base_, ptr);
        }

        const char16_t* rawCharPtrAt(size_t offset) const {
            MOZ_ASSERT(startOffset_ <= offset);
            MOZ_ASSERT(offset - startOffset_ <= mozilla::PointerRangeSize(base_, limit_));
            return base_ + (offset - startOffset_);
        }

        const char16_t* limit() const {
            return limit_;
        }

        char16_t getRawChar() {
            return *ptr++;      
        }

        char16_t peekRawChar() const {
            return *ptr;        
        }

        bool matchRawChar(char16_t c) {
            if (*ptr == c) {    
                ptr++;
                return true;
            }
            return false;
        }

        bool matchRawCharBackwards(char16_t c) {
            MOZ_ASSERT(ptr);     
            if (*(ptr - 1) == c) {
                ptr--;
                return true;
            }
            return false;
        }

        void ungetRawChar() {
            MOZ_ASSERT(ptr);     
            ptr--;
        }

        const char16_t* addressOfNextRawChar(bool allowPoisoned = false) const {
            MOZ_ASSERT_IF(!allowPoisoned, ptr);     
            return ptr;
        }

        
        void setAddressOfNextRawChar(const char16_t* a, bool allowPoisoned = false) {
            MOZ_ASSERT_IF(!allowPoisoned, a);
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

        
        
        size_t findEOLMax(size_t start, size_t max);

      private:
        const char16_t* base_;          
        uint32_t startOffset_;          
        const char16_t* limit_;         
        const char16_t* ptr;            
    };

    bool getTokenInternal(TokenKind* ttp, Modifier modifier);

    bool getBracedUnicode(uint32_t* code);
    bool getStringOrTemplateToken(int untilChar, Token** tp);

    int32_t getChar();
    int32_t getCharIgnoreEOL();
    void ungetChar(int32_t c);
    void ungetCharIgnoreEOL(int32_t c);
    Token* newToken(ptrdiff_t adjust);
    bool peekUnicodeEscape(int32_t* c);
    bool matchUnicodeEscapeIdStart(int32_t* c);
    bool matchUnicodeEscapeIdent(int32_t* c);
    bool peekChars(int n, char16_t* cp);

    bool getDirectives(bool isMultiline, bool shouldWarnDeprecated);
    bool getDirective(bool isMultiline, bool shouldWarnDeprecated,
                      const char* directive, int directiveLength,
                      const char* errorMsgPragma,
                      mozilla::UniquePtr<char16_t[], JS::FreePolicy>* destination);
    bool getDisplayURL(bool isMultiline, bool shouldWarnDeprecated);
    bool getSourceMappingURL(bool isMultiline, bool shouldWarnDeprecated);

    
    bool matchChar(int32_t expect) {
        MOZ_ASSERT(!TokenBuf::isRawEOLChar(expect));
        return MOZ_LIKELY(userbuf.hasRawChars()) &&
               userbuf.matchRawChar(expect);
    }

    void consumeKnownChar(int32_t expect) {
        mozilla::DebugOnly<int32_t> c = getChar();
        MOZ_ASSERT(c == expect);
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

    
    const ReadOnlyCompileOptions& options_;

    Token               tokens[ntokens];    
    unsigned            cursor;             
    unsigned            lookahead;          
    unsigned            lineno;             
    Flags               flags;              
    size_t              linebase;           
    size_t              prevLinebase;       
    TokenBuf            userbuf;            
    const char*         filename;          
    mozilla::UniquePtr<char16_t[], JS::FreePolicy> displayURL_; 
    mozilla::UniquePtr<char16_t[], JS::FreePolicy> sourceMapURL_; 
    CharBuffer          tokenbuf;           
    uint8_t             isExprEnding[TOK_LIMIT];
    ExclusiveContext*   const cx;
    bool                mutedErrors;
    StrictModeGetter*   strictModeGetter;  
};



#define JSREPORT_UC 0x100

extern const char*
TokenKindToDesc(TokenKind tt);

} 
} 

extern JS_FRIEND_API(int)
js_fgets(char* buf, int size, FILE* file);

#ifdef DEBUG
extern const char*
TokenKindToString(js::frontend::TokenKind tt);
#endif

#endif 
