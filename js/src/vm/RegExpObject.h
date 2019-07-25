







































#ifndef RegExpObject_h__
#define RegExpObject_h__

#include "mozilla/Attributes.h"

#include <stddef.h>
#include "jsobj.h"

#include "js/TemplateLib.h"

#include "yarr/Yarr.h"
#if ENABLE_YARR_JIT
#include "yarr/YarrJIT.h"
#include "yarr/YarrSyntaxChecker.h"
#else
#include "yarr/pcre/pcre.h"
#endif





















namespace js {

enum RegExpRunStatus
{
    RegExpRunStatus_Error,
    RegExpRunStatus_Success,
    RegExpRunStatus_Success_NotFound
};

class RegExpObjectBuilder
{
    JSContext       *cx;
    RegExpObject    *reobj_;

    bool getOrCreate();
    bool getOrCreateClone(RegExpObject *proto);

  public:
    RegExpObjectBuilder(JSContext *cx, RegExpObject *reobj = NULL);

    RegExpObject *reobj() { return reobj_; }

    RegExpObject *build(JSAtom *source, RegExpFlag flags);
    RegExpObject *build(JSAtom *source, RegExpShared &shared);

    
    RegExpObject *clone(RegExpObject *other, RegExpObject *proto);
};

JSObject *
CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);

namespace detail {

class RegExpCode
{
#if ENABLE_YARR_JIT
    typedef JSC::Yarr::BytecodePattern BytecodePattern;
    typedef JSC::Yarr::ErrorCode ErrorCode;
    typedef JSC::Yarr::JSGlobalData JSGlobalData;
    typedef JSC::Yarr::YarrCodeBlock YarrCodeBlock;
    typedef JSC::Yarr::YarrPattern YarrPattern;

    
    YarrCodeBlock   codeBlock;
    BytecodePattern *byteCode;
#else
    JSRegExp        *compiled;
#endif

  public:
    RegExpCode()
      :
#if ENABLE_YARR_JIT
        codeBlock(),
        byteCode(NULL)
#else
        compiled(NULL)
#endif
    { }

    ~RegExpCode() {
#if ENABLE_YARR_JIT
        codeBlock.release();
        if (byteCode)
            Foreground::delete_<BytecodePattern>(byteCode);
#else
        if (compiled)
            jsRegExpFree(compiled);
#endif
    }

    static bool checkSyntax(JSContext *cx, TokenStream *tokenStream, JSLinearString *source) {
#if ENABLE_YARR_JIT
        ErrorCode error = JSC::Yarr::checkSyntax(*source);
        if (error == JSC::Yarr::NoError)
            return true;

        reportYarrError(cx, tokenStream, error);
        return false;
#else
# error "Syntax checking not implemented for !ENABLE_YARR_JIT"
#endif
    }

#if ENABLE_YARR_JIT
    static inline bool isJITRuntimeEnabled(JSContext *cx);
    static void reportYarrError(JSContext *cx, TokenStream *ts, JSC::Yarr::ErrorCode error);
#else
    static void reportPCREError(JSContext *cx, int error);
#endif

    static size_t getOutputSize(size_t pairCount) {
#if ENABLE_YARR_JIT
        return pairCount * 2;
#else
        return pairCount * 3; 
#endif
    }

    bool compile(JSContext *cx, JSLinearString &pattern, uintN *parenCount, RegExpFlag flags);


    RegExpRunStatus
    execute(JSContext *cx, const jschar *chars, size_t length, size_t start,
            int *output, size_t outputCount);
};

}  





















class RegExpShared
{
    friend class RegExpCompartment;
    friend class RegExpGuard;

    detail::RegExpCode code;
    uintN              parenCount;
    RegExpFlag         flags;
    size_t             activeUseCount;   
    uint64_t           gcNumberWhenUsed; 

    bool compile(JSContext *cx, JSAtom *source);

    RegExpShared(JSRuntime *rt, RegExpFlag flags);
    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;

  public:

    
    inline void prepareForUse(JSContext *cx);

    

    RegExpRunStatus
    execute(JSContext *cx, const jschar *chars, size_t length, size_t *lastIndex,
            MatchPairs **output);

    

    size_t getParenCount() const        { return parenCount; }

    
    size_t pairCount() const            { return parenCount + 1; }

    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const             { return flags & IgnoreCaseFlag; }
    bool global() const                 { return flags & GlobalFlag; }
    bool multiline() const              { return flags & MultilineFlag; }
    bool sticky() const                 { return flags & StickyFlag; }
};





class RegExpGuard
{
    RegExpShared *re_;
    RegExpGuard(const RegExpGuard &) MOZ_DELETE;
    void operator=(const RegExpGuard &) MOZ_DELETE;
  public:
    RegExpGuard() : re_(NULL) {}
    RegExpGuard(RegExpShared &re) : re_(&re) {
        re_->activeUseCount++;
    }
    void init(RegExpShared &re) {
        JS_ASSERT(!re_);
        re_ = &re;
        re_->activeUseCount++;
    }
    ~RegExpGuard() {
        if (re_) {
            JS_ASSERT(re_->activeUseCount > 0);
            re_->activeUseCount--;
        }
    }
    bool initialized() const { return !!re_; }
    RegExpShared *operator->() { JS_ASSERT(initialized()); return re_; }
    RegExpShared &operator*() { JS_ASSERT(initialized()); return *re_; }
};

class RegExpCompartment
{
    enum Type { Normal = 0x0, Hack = 0x1 };

    struct Key {
        JSAtom *atom;
        uint16_t flag;
        uint16_t type;
        Key() {}
        Key(JSAtom *atom, RegExpFlag flag, Type type)
          : atom(atom), flag(flag), type(type) {}
        typedef Key Lookup;
        static HashNumber hash(const Lookup &l) {
            return DefaultHasher<JSAtom *>::hash(l.atom) ^ (l.flag << 1) ^ l.type;
        }
        static bool match(Key l, Key r) {
            return l.atom == r.atom && l.flag == r.flag && l.type == r.type;
        }
    };

    typedef HashMap<Key, RegExpShared *, Key, RuntimeAllocPolicy> Map;
    Map map_;

    bool get(JSContext *cx, JSAtom *key, JSAtom *source, RegExpFlag flags, Type type,
             RegExpGuard *g);

  public:
    RegExpCompartment(JSRuntime *rt);
    ~RegExpCompartment();

    bool init(JSContext *cx);
    void sweep(JSRuntime *rt);

    
    bool get(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g);

    
    bool get(JSContext *cx, JSAtom *source, JSString *maybeOpt, RegExpGuard *g);

    










    bool getHack(JSContext *cx, JSAtom *source, JSAtom *hackedSource, RegExpFlag flags,
                 RegExpGuard *g);

    




    bool lookupHack(JSAtom *source, RegExpFlag flags, JSContext *cx, RegExpGuard *g);
};

class RegExpObject : public JSObject
{
    typedef detail::RegExpCode RegExpCode;

    static const uintN LAST_INDEX_SLOT          = 0;
    static const uintN SOURCE_SLOT              = 1;
    static const uintN GLOBAL_FLAG_SLOT         = 2;
    static const uintN IGNORE_CASE_FLAG_SLOT    = 3;
    static const uintN MULTILINE_FLAG_SLOT      = 4;
    static const uintN STICKY_FLAG_SLOT         = 5;

  public:
    static const uintN RESERVED_SLOTS = 6;

    




    static RegExpObject *
    create(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
           RegExpFlag flags, TokenStream *ts);

    static RegExpObject *
    createNoStatics(JSContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                    TokenStream *ts);

    static RegExpObject *
    createNoStatics(JSContext *cx, JSAtom *atom, RegExpFlag flags, TokenStream *ts);

    










    RegExpRunStatus
    execute(JSContext *cx, const jschar *chars, size_t length, size_t *lastIndex,
            MatchPairs **output);

    

    const Value &getLastIndex() const {
        return getSlot(LAST_INDEX_SLOT);
    }
    inline void setLastIndex(const Value &v);
    inline void setLastIndex(double d);
    inline void zeroLastIndex();

    JSFlatString *toString(JSContext *cx) const;

    JSAtom *getSource() const {
        return &getSlot(SOURCE_SLOT).toString()->asAtom();
    }
    inline void setSource(JSAtom *source);

    RegExpFlag getFlags() const {
        uintN flags = 0;
        flags |= global() ? GlobalFlag : 0;
        flags |= ignoreCase() ? IgnoreCaseFlag : 0;
        flags |= multiline() ? MultilineFlag : 0;
        flags |= sticky() ? StickyFlag : 0;
        return RegExpFlag(flags);
    }

    

    inline void setIgnoreCase(bool enabled);
    inline void setGlobal(bool enabled);
    inline void setMultiline(bool enabled);
    inline void setSticky(bool enabled);
    bool ignoreCase() const { return getSlot(IGNORE_CASE_FLAG_SLOT).toBoolean(); }
    bool global() const     { return getSlot(GLOBAL_FLAG_SLOT).toBoolean(); }
    bool multiline() const  { return getSlot(MULTILINE_FLAG_SLOT).toBoolean(); }
    bool sticky() const     { return getSlot(STICKY_FLAG_SLOT).toBoolean(); }

    inline void shared(RegExpGuard *g) const;
    inline bool getShared(JSContext *cx, RegExpGuard *g);
    inline void setShared(JSContext *cx, RegExpShared &shared);

  private:
    friend class RegExpObjectBuilder;

    




    Shape *assignInitialShape(JSContext *cx);

    inline bool init(JSContext *cx, JSAtom *source, RegExpFlag flags);

    



    bool createShared(JSContext *cx, RegExpGuard *g);
    RegExpShared *maybeShared() const;

    RegExpObject() MOZ_DELETE;
    RegExpObject &operator=(const RegExpObject &reo) MOZ_DELETE;

    
    void setPrivate(void *priv) MOZ_DELETE;
};







bool
ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut);









inline bool
RegExpToShared(JSContext *cx, JSObject &obj, RegExpGuard *g);

bool
XDRScriptRegExpObject(JSXDRState *xdr, HeapPtrObject *objp);

} 

#endif
