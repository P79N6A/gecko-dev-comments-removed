







































#ifndef RegExpObject_h__
#define RegExpObject_h__

#include <stddef.h>
#include "jsobj.h"
#include "jstl.h"

#include "yarr/Yarr.h"
#if ENABLE_YARR_JIT
#include "yarr/YarrJIT.h"
#else
#include "yarr/pcre/pcre.h"
#endif

namespace js {

class RegExpPrivate;

class RegExpObject : public ::JSObject
{
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
           TokenStream *ts);

    static inline RegExpObject *
    createNoStatics(JSContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                    TokenStream *ts);

    static RegExpObject *clone(JSContext *cx, RegExpObject *obj, RegExpObject *proto);

    
    JSFlatString *toString(JSContext *cx) const;

    

    const Value &getLastIndex() const {
        return getSlot(LAST_INDEX_SLOT);
    }
    void setLastIndex(const Value &v) {
        setSlot(LAST_INDEX_SLOT, v);
    }
    void setLastIndex(double d) {
        setSlot(LAST_INDEX_SLOT, NumberValue(d));
    }
    void zeroLastIndex() {
        setSlot(LAST_INDEX_SLOT, Int32Value(0));
    }

    JSString *getSource() const {
        return getSlot(SOURCE_SLOT).toString();
    }
    void setSource(JSString *source) {
        setSlot(SOURCE_SLOT, StringValue(source));
    }
    RegExpFlag getFlags() const {
        uintN flags = 0;
        flags |= getSlot(GLOBAL_FLAG_SLOT).toBoolean() ? GlobalFlag : 0;
        flags |= getSlot(IGNORE_CASE_FLAG_SLOT).toBoolean() ? IgnoreCaseFlag : 0;
        flags |= getSlot(MULTILINE_FLAG_SLOT).toBoolean() ? MultilineFlag : 0;
        flags |= getSlot(STICKY_FLAG_SLOT).toBoolean() ? StickyFlag : 0;
        return RegExpFlag(flags);
    }
    inline RegExpPrivate *getPrivate() const;

    

    void setIgnoreCase(bool enabled)    { setSlot(IGNORE_CASE_FLAG_SLOT, BooleanValue(enabled)); }
    void setGlobal(bool enabled)        { setSlot(GLOBAL_FLAG_SLOT, BooleanValue(enabled)); }
    void setMultiline(bool enabled)     { setSlot(MULTILINE_FLAG_SLOT, BooleanValue(enabled)); }
    void setSticky(bool enabled)        { setSlot(STICKY_FLAG_SLOT, BooleanValue(enabled)); }

  private:
    
    inline bool reset(JSContext *cx, AlreadyIncRefed<RegExpPrivate> rep);

    friend bool ResetRegExpObject(JSContext *, RegExpObject *, JSString *, RegExpFlag);
    friend bool ResetRegExpObject(JSContext *, RegExpObject *, AlreadyIncRefed<RegExpPrivate>);

    




    const Shape *assignInitialShape(JSContext *cx);

    RegExpObject();
    RegExpObject &operator=(const RegExpObject &reo);
}; 

inline bool
ResetRegExpObject(JSContext *cx, RegExpObject *reobj, JSString *str, RegExpFlag flags);


inline bool
ResetRegExpObject(JSContext *cx, AlreadyIncRefed<RegExpPrivate> rep);















class RegExpPrivate
{
#if ENABLE_YARR_JIT
    
    JSC::Yarr::YarrCodeBlock    codeBlock;
    JSC::Yarr::BytecodePattern  *byteCode;
#else
    JSRegExp                    *compiled;
#endif

    JSLinearString              *source;
    size_t                      refCount;
    unsigned                    parenCount; 
    RegExpFlag                  flags;
#ifdef DEBUG
  public:
    JSCompartment               *compartment;

  private:
#endif

    RegExpPrivate(JSLinearString *source, RegExpFlag flags, JSCompartment *compartment)
      :
#if ENABLE_YARR_JIT
        codeBlock(),
        byteCode(NULL),
#else
        compiled(NULL),
#endif
        source(source), refCount(1), parenCount(0), flags(flags)
#ifdef DEBUG
        , compartment(compartment)
#endif
    { }

    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;

    ~RegExpPrivate() {
#if ENABLE_YARR_JIT
        codeBlock.release();
        if (byteCode)
            Foreground::delete_<JSC::Yarr::BytecodePattern>(byteCode);
#else
        if (compiled)
            jsRegExpFree(compiled);
#endif
    }

    bool compileHelper(JSContext *cx, JSLinearString &pattern, TokenStream *ts);
    bool compile(JSContext *cx, TokenStream *ts);
#if !ENABLE_YARR_JIT
    void reportPCREError(JSContext *cx, int error);
#endif
    void reportYarrError(JSContext *cx, TokenStream *ts, JSC::Yarr::ErrorCode error);
    static inline void checkMatchPairs(JSString *input, int *buf, size_t matchItemCount);
    static JSObject *createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount);
    bool executeInternal(JSContext *cx, RegExpStatics *res, JSString *input,
                         size_t *lastIndex, bool test, Value *rval);

  public:
    






    bool execute(JSContext *cx, RegExpStatics *res, JSString *input, size_t *lastIndex, bool test,
                 Value *rval) {
        JS_ASSERT(res);
        return executeInternal(cx, res, input, lastIndex, test, rval);
    }

    bool executeNoStatics(JSContext *cx, JSString *input, size_t *lastIndex, bool test,
                          Value *rval) {
        return executeInternal(cx, NULL, input, lastIndex, test, rval);
    }

    

    static AlreadyIncRefed<RegExpPrivate> create(JSContext *cx, JSString *source, RegExpFlag flags,
                                                 TokenStream *ts);

    
    static AlreadyIncRefed<RegExpPrivate> createFlagged(JSContext *cx, JSString *source,
                                                        JSString *flags, TokenStream *ts);

    

    void incref(JSContext *cx);
    void decref(JSContext *cx);

    

    JSLinearString *getSource() const { return source; }
    size_t getParenCount() const { return parenCount; }
    RegExpFlag getFlags() const { return flags; }
    bool ignoreCase() const { return flags & IgnoreCaseFlag; }
    bool global() const     { return flags & GlobalFlag; }
    bool multiline() const  { return flags & MultilineFlag; }
    bool sticky() const     { return flags & StickyFlag; }
}; 







bool
ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut);

inline bool
ValueIsRegExp(const Value &v);

inline bool
IsRegExpMetaChar(jschar c);

} 

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);

JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

#endif
