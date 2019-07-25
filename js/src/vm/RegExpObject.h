







































#ifndef RegExpObject_h__
#define RegExpObject_h__

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

class RegExpObject : public ::JSObject
{
    typedef detail::RegExpPrivate RegExpPrivate;
    typedef detail::RegExpPrivateCode RegExpPrivateCode;

    static const uintN LAST_INDEX_SLOT          = 0;
    static const uintN SOURCE_SLOT              = 1;
    static const uintN GLOBAL_FLAG_SLOT         = 2;
    static const uintN IGNORE_CASE_FLAG_SLOT    = 3;
    static const uintN MULTILINE_FLAG_SLOT      = 4;
    static const uintN STICKY_FLAG_SLOT         = 5;

  public:
    static const uintN RESERVED_SLOTS = 6;

    




    static inline RegExpObject *
    create(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length, RegExpFlag flags,
           TokenStream *tokenStream);

    static inline RegExpObject *
    createNoStatics(JSContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                    TokenStream *tokenStream);

    static inline RegExpObject *
    createNoStatics(JSContext *cx, JSAtom *atom, RegExpFlag flags, TokenStream *tokenStream);

    
    JSFlatString *toString(JSContext *cx) const;

    










    RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length, size_t *lastIndex,
                            LifoAllocScope &allocScope, MatchPairs **output);

    

    const Value &getLastIndex() const {
        return getSlot(LAST_INDEX_SLOT);
    }
    inline void setLastIndex(const Value &v);
    inline void setLastIndex(double d);
    inline void zeroLastIndex();

    JSLinearString *getSource() const {
        return &getSlot(SOURCE_SLOT).toString()->asLinear();
    }
    inline void setSource(JSLinearString *source);

    RegExpFlag getFlags() const {
        uintN flags = 0;
        flags |= global() ? GlobalFlag : 0;
        flags |= ignoreCase() ? IgnoreCaseFlag : 0;
        flags |= multiline() ? MultilineFlag : 0;
        flags |= sticky() ? StickyFlag : 0;
        return RegExpFlag(flags);
    }

    

    inline size_t *addressOfPrivateRefCount() const;

    

    inline void setIgnoreCase(bool enabled);
    inline void setGlobal(bool enabled);
    inline void setMultiline(bool enabled);
    inline void setSticky(bool enabled);
    bool ignoreCase() const { return getSlot(IGNORE_CASE_FLAG_SLOT).toBoolean(); }
    bool global() const     { return getSlot(GLOBAL_FLAG_SLOT).toBoolean(); }
    bool multiline() const  { return getSlot(MULTILINE_FLAG_SLOT).toBoolean(); }
    bool sticky() const     { return getSlot(STICKY_FLAG_SLOT).toBoolean(); }

    inline void finalize(JSContext *cx);

    
    inline void purge(JSContext *x);

    



    bool makePrivateNow(JSContext *cx) {
        return getPrivate() ? true : !!makePrivate(cx);
    }

  private:
    friend class RegExpObjectBuilder;
    friend class RegExpMatcher;

    inline bool init(JSContext *cx, JSLinearString *source, RegExpFlag flags);

    RegExpPrivate *getOrCreatePrivate(JSContext *cx) {
        if (RegExpPrivate *rep = getPrivate())
            return rep;

        return makePrivate(cx);
    }

    
    RegExpPrivate *getPrivate() const {
        return static_cast<RegExpPrivate *>(JSObject::getPrivate());
    }

    inline void setPrivate(RegExpPrivate *rep);

    



    RegExpPrivate *makePrivate(JSContext *cx);

    friend bool ResetRegExpObject(JSContext *, RegExpObject *, JSLinearString *, RegExpFlag);
    friend bool ResetRegExpObject(JSContext *, RegExpObject *, AlreadyIncRefed<RegExpPrivate>);

    




    Shape *assignInitialShape(JSContext *cx);

    RegExpObject();
    RegExpObject &operator=(const RegExpObject &reo);
}; 


class RegExpObjectBuilder
{
    typedef detail::RegExpPrivate RegExpPrivate;

    JSContext       *cx;
    RegExpObject    *reobj_;

    bool getOrCreate();
    bool getOrCreateClone(RegExpObject *proto);

    RegExpObject *build(AlreadyIncRefed<RegExpPrivate> rep);

  public:
    RegExpObjectBuilder(JSContext *cx, RegExpObject *reobj = NULL)
      : cx(cx), reobj_(reobj)
    { }

    RegExpObject *reobj() { return reobj_; }

    RegExpObject *build(JSLinearString *str, RegExpFlag flags);
    RegExpObject *build(RegExpObject *other);

    
    RegExpObject *clone(RegExpObject *other, RegExpObject *proto);
};

namespace detail {


class RegExpPrivateCode
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
    RegExpPrivateCode()
      :
#if ENABLE_YARR_JIT
        codeBlock(),
        byteCode(NULL)
#else
        compiled(NULL)
#endif
    { }

    ~RegExpPrivateCode() {
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

    inline bool compile(JSContext *cx, JSLinearString &pattern, TokenStream *ts, uintN *parenCount,
                        RegExpFlag flags);


    inline RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length, size_t start,
                                   int *output, size_t outputCount);
};















class RegExpPrivate
{
    RegExpPrivateCode   code;
    JSLinearString      *source;
    size_t              refCount;
    uintN               parenCount;
    RegExpFlag          flags;

  private:
    RegExpPrivate(JSLinearString *source, RegExpFlag flags)
      : source(source), refCount(1), parenCount(0), flags(flags)
    { }

    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;

    bool compile(JSContext *cx, TokenStream *ts);
    static inline void checkMatchPairs(JSString *input, int *buf, size_t matchItemCount);

    static RegExpPrivate *
    createUncached(JSContext *cx, JSLinearString *source, RegExpFlag flags,
                   TokenStream *tokenStream);

    static RegExpPrivateCache *getOrCreateCache(JSContext *cx);
    static bool cacheLookup(JSContext *cx, JSAtom *atom, RegExpFlag flags,
                            AlreadyIncRefed<RegExpPrivate> *result);
    static bool cacheInsert(JSContext *cx, JSAtom *atom, RegExpPrivate *priv);

  public:
    static AlreadyIncRefed<RegExpPrivate>
    create(JSContext *cx, JSLinearString *source, RegExpFlag flags, TokenStream *ts);

    static AlreadyIncRefed<RegExpPrivate>
    create(JSContext *cx, JSLinearString *source, JSString *flags, TokenStream *ts);

    RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length, size_t *lastIndex,
                            LifoAllocScope &allocScope, MatchPairs **output);

    

    void incref(JSContext *cx);
    void decref(JSContext *cx);

    
    size_t *addressOfRefCount() { return &refCount; }

    

    JSLinearString *getSource() const   { return source; }
    size_t getParenCount() const        { return parenCount; }

    
    size_t pairCount() const            { return parenCount + 1; }

    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const { return flags & IgnoreCaseFlag; }
    bool global() const     { return flags & GlobalFlag; }
    bool multiline() const  { return flags & MultilineFlag; }
    bool sticky() const     { return flags & StickyFlag; }
};

} 









class RegExpMatcher
{
    typedef detail::RegExpPrivate RegExpPrivate;

    JSContext                   *cx;
    AutoRefCount<RegExpPrivate> arc;

  public:
    explicit RegExpMatcher(JSContext *cx)
      : cx(cx), arc(cx)
    { }

    bool null() const {
        return arc.null();
    }
    bool global() const {
        return arc.get()->global();
    }
    bool sticky() const {
        return arc.get()->sticky();
    }

    bool reset(RegExpObject *reobj) {
        RegExpPrivate *priv = reobj->getOrCreatePrivate(cx);
        if (!priv)
            return false;
        arc.reset(NeedsIncRef<RegExpPrivate>(priv));
        return true;
    }

    inline bool reset(JSLinearString *patstr, JSString *opt);

    RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length, size_t *lastIndex,
                            LifoAllocScope &allocScope, MatchPairs **output) {
        JS_ASSERT(!arc.null());
        return arc.get()->execute(cx, chars, length, lastIndex, allocScope, output);
    }
};







bool
ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut);

inline bool
ValueIsRegExp(const Value &v);

inline bool
IsRegExpMetaChar(jschar c);

inline bool
CheckRegExpSyntax(JSContext *cx, JSLinearString *str)
{
    return detail::RegExpPrivateCode::checkSyntax(cx, NULL, str);
}

} 

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);

JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

#endif
