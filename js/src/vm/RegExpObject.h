





#ifndef vm_RegExpObject_h
#define vm_RegExpObject_h

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jsproxy.h"

#include "gc/Marking.h"
#include "gc/Zone.h"
#include "vm/Shape.h"

#ifdef JS_YARR
#ifdef JS_ION
#include "yarr/YarrJIT.h"
#endif
#include "yarr/YarrInterpreter.h"
#endif 



















namespace js {

class MatchPair;
class MatchPairs;
class RegExpShared;

namespace frontend { class TokenStream; }

enum RegExpFlag
{
    IgnoreCaseFlag  = 0x01,
    GlobalFlag      = 0x02,
    MultilineFlag   = 0x04,
    StickyFlag      = 0x08,

    NoFlags         = 0x00,
    AllFlags        = 0x0f
};

enum RegExpRunStatus
{
    RegExpRunStatus_Error,
    RegExpRunStatus_Success,
    RegExpRunStatus_Success_NotFound
};

class RegExpObjectBuilder
{
    ExclusiveContext *cx;
    Rooted<RegExpObject*> reobj_;

    bool getOrCreate();
    bool getOrCreateClone(HandleTypeObject type);

  public:
    RegExpObjectBuilder(ExclusiveContext *cx, RegExpObject *reobj = nullptr);

    RegExpObject *reobj() { return reobj_; }

    RegExpObject *build(HandleAtom source, RegExpFlag flags);
    RegExpObject *build(HandleAtom source, RegExpShared &shared);

    
    RegExpObject *clone(Handle<RegExpObject*> other);
};

JSObject *
CloneRegExpObject(JSContext *cx, JSObject *obj);































class RegExpShared
{
    friend class RegExpCompartment;
    friend class RegExpStatics;
    friend class RegExpGuard;

    typedef frontend::TokenStream TokenStream;

#ifdef JS_YARR
    typedef JSC::Yarr::BytecodePattern BytecodePattern;
    typedef JSC::Yarr::ErrorCode ErrorCode;
    typedef JSC::Yarr::YarrPattern YarrPattern;
#ifdef JS_ION
    typedef JSC::Yarr::JSGlobalData JSGlobalData;
    typedef JSC::Yarr::YarrCodeBlock YarrCodeBlock;
    typedef JSC::Yarr::YarrJITCompileMode YarrJITCompileMode;
#endif
#endif

    




    JSAtom *           source;

    RegExpFlag         flags;
    size_t             parenCount;
    bool               canStringMatch;

#ifdef JS_YARR

#ifdef JS_ION
    
    YarrCodeBlock   codeBlock;
#endif
    BytecodePattern *bytecode;

#else 

#ifdef JS_ION
    HeapPtrJitCode     jitCode;
#endif
    uint8_t            *byteCode;

#endif 

    
    Vector<uint8_t *, 0, SystemAllocPolicy> tables;

    
    size_t             activeUseCount;
    uint64_t           gcNumberWhenUsed;

    
    bool compile(JSContext *cx, bool matchOnly, const jschar *sampleChars, size_t sampleLength);
    bool compile(JSContext *cx, HandleAtom pattern, bool matchOnly, const jschar *sampleChars, size_t sampleLength);

    bool compileIfNecessary(JSContext *cx, const jschar *sampleChars, size_t sampleLength);

#ifdef JS_YARR
    bool compileMatchOnlyIfNecessary(JSContext *cx);
#endif

  public:
    RegExpShared(JSAtom *source, RegExpFlag flags, uint64_t gcNumber);
    ~RegExpShared();

#ifdef JS_YARR
    

    
    static bool isJITRuntimeEnabled(JSContext *cx) {
        #ifdef JS_ION
        # if defined(ANDROID)
            return !cx->jitIsBroken;
        # else
            return true;
        # endif
        #else
            return false;
        #endif
    }
    static void reportYarrError(ExclusiveContext *cx, TokenStream *ts, ErrorCode error);
    static bool checkSyntax(ExclusiveContext *cx, TokenStream *tokenStream, JSLinearString *source);
#endif 

    
    void prepareForUse(ExclusiveContext *cx) {
        gcNumberWhenUsed = cx->zone()->gcNumber();
        JSString::writeBarrierPre(source);
#ifndef JS_YARR
#ifdef JS_ION
        if (jitCode)
            jit::JitCode::writeBarrierPre(jitCode);
#endif
#endif 
    }

    
    RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length,
                            size_t *lastIndex, MatchPairs &matches);

#ifdef JS_YARR
    
    RegExpRunStatus executeMatchOnly(JSContext *cx, const jschar *chars, size_t length,
                                     size_t *lastIndex, MatchPair &match);
#endif

    
    bool addTable(uint8_t *table) {
        return tables.append(table);
    }

    

    size_t getParenCount() const {
        JS_ASSERT(isCompiled(true) || isCompiled(false) || canStringMatch);
        return parenCount;
    }

    void incRef()                       { activeUseCount++; }
    void decRef()                       { JS_ASSERT(activeUseCount > 0); activeUseCount--; }

    
    size_t pairCount() const            { return getParenCount() + 1; }

    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const             { return flags & IgnoreCaseFlag; }
    bool global() const                 { return flags & GlobalFlag; }
    bool multiline() const              { return flags & MultilineFlag; }
    bool sticky() const                 { return flags & StickyFlag; }

#ifdef JS_YARR

    bool hasCode(bool matchOnly) const {
#ifdef JS_ION
        return matchOnly ? codeBlock.has16BitCodeMatchOnly() : codeBlock.has16BitCode();
#else
        return false;
#endif
    }
    bool hasBytecode() const {
        return bytecode != nullptr;
    }
    bool isCompiled(bool matchOnly) const {
        return hasBytecode() || hasCode(matchOnly);
    }

#else 

    bool hasJitCode() const {
#ifdef JS_ION
        return jitCode != nullptr;
#else
        return false;
#endif
    }
    bool hasByteCode() const {
        return byteCode != nullptr;
    }

    bool isCompiled(bool matchOnly) const {
        return hasJitCode() || hasByteCode();
    }

#endif 

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};





class RegExpGuard : public JS::CustomAutoRooter
{
    RegExpShared *re_;

    RegExpGuard(const RegExpGuard &) MOZ_DELETE;
    void operator=(const RegExpGuard &) MOZ_DELETE;

  public:
    RegExpGuard(ExclusiveContext *cx)
      : CustomAutoRooter(cx), re_(nullptr)
    {}

    RegExpGuard(ExclusiveContext *cx, RegExpShared &re)
      : CustomAutoRooter(cx), re_(nullptr)
    {
        init(re);
    }

    ~RegExpGuard() {
        release();
    }

  public:
    void init(RegExpShared &re) {
        JS_ASSERT(!initialized());
        re_ = &re;
        re_->incRef();
    }

    void release() {
        if (re_) {
            re_->decRef();
            re_ = nullptr;
        }
    }

    virtual void trace(JSTracer *trc) {
        if (!re_)
            return;
        if (re_->source) {
            MarkStringRoot(trc, reinterpret_cast<JSString**>(&re_->source),
                           "RegExpGuard source");
        }
#ifndef JS_YARR
#ifdef JS_ION
        if (re_->jitCode) {
            MarkJitCodeRoot(trc, reinterpret_cast<jit::JitCode**>(&re_->jitCode),
                            "RegExpGuard code");
        }
#endif
#endif 
    }

    bool initialized() const { return !!re_; }
    RegExpShared *re() const { JS_ASSERT(initialized()); return re_; }
    RegExpShared *operator->() { return re(); }
    RegExpShared &operator*() { return *re(); }
};

class RegExpCompartment
{
    struct Key {
        JSAtom *atom;
        uint16_t flag;

        Key() {}
        Key(JSAtom *atom, RegExpFlag flag)
          : atom(atom), flag(flag)
        { }

        typedef Key Lookup;
        static HashNumber hash(const Lookup &l) {
            return DefaultHasher<JSAtom *>::hash(l.atom) ^ (l.flag << 1);
        }
        static bool match(Key l, Key r) {
            return l.atom == r.atom && l.flag == r.flag;
        }
    };

    



    typedef HashMap<Key, RegExpShared *, Key, RuntimeAllocPolicy> Map;
    Map map_;

    




    typedef HashSet<RegExpShared *, DefaultHasher<RegExpShared*>, RuntimeAllocPolicy> PendingSet;
    PendingSet inUse_;

    




    ReadBarrieredObject matchResultTemplateObject_;

    JSObject *createMatchResultTemplateObject(JSContext *cx);

  public:
    RegExpCompartment(JSRuntime *rt);
    ~RegExpCompartment();

    bool init(JSContext *cx);
    void sweep(JSRuntime *rt);
    void clearTables();

    bool get(ExclusiveContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g);

    
    bool get(JSContext *cx, HandleAtom source, JSString *maybeOpt, RegExpGuard *g);

    
    JSObject *getOrCreateMatchResultTemplateObject(JSContext *cx) {
        if (matchResultTemplateObject_)
            return matchResultTemplateObject_;
        return createMatchResultTemplateObject(cx);
    }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

class RegExpObject : public JSObject
{
    static const unsigned LAST_INDEX_SLOT          = 0;
    static const unsigned SOURCE_SLOT              = 1;
    static const unsigned GLOBAL_FLAG_SLOT         = 2;
    static const unsigned IGNORE_CASE_FLAG_SLOT    = 3;
    static const unsigned MULTILINE_FLAG_SLOT      = 4;
    static const unsigned STICKY_FLAG_SLOT         = 5;

  public:
    static const unsigned RESERVED_SLOTS = 6;

    static const Class class_;

    




    static RegExpObject *
    create(ExclusiveContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
           RegExpFlag flags, frontend::TokenStream *ts, LifoAlloc &alloc);

    static RegExpObject *
    createNoStatics(ExclusiveContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                    frontend::TokenStream *ts, LifoAlloc &alloc);

    static RegExpObject *
    createNoStatics(ExclusiveContext *cx, HandleAtom atom, RegExpFlag flags,
                    frontend::TokenStream *ts, LifoAlloc &alloc);

    

    static unsigned lastIndexSlot() { return LAST_INDEX_SLOT; }

    const Value &getLastIndex() const { return getSlot(LAST_INDEX_SLOT); }

    void setLastIndex(double d) {
        setSlot(LAST_INDEX_SLOT, NumberValue(d));
    }

    void zeroLastIndex() {
        setSlot(LAST_INDEX_SLOT, Int32Value(0));
    }

    JSFlatString *toString(JSContext *cx) const;

    JSAtom *getSource() const { return &getSlot(SOURCE_SLOT).toString()->asAtom(); }

    void setSource(JSAtom *source) {
        setSlot(SOURCE_SLOT, StringValue(source));
    }

    RegExpFlag getFlags() const {
        unsigned flags = 0;
        flags |= global() ? GlobalFlag : 0;
        flags |= ignoreCase() ? IgnoreCaseFlag : 0;
        flags |= multiline() ? MultilineFlag : 0;
        flags |= sticky() ? StickyFlag : 0;
        return RegExpFlag(flags);
    }

    

    void setIgnoreCase(bool enabled) {
        setSlot(IGNORE_CASE_FLAG_SLOT, BooleanValue(enabled));
    }

    void setGlobal(bool enabled) {
        setSlot(GLOBAL_FLAG_SLOT, BooleanValue(enabled));
    }

    void setMultiline(bool enabled) {
        setSlot(MULTILINE_FLAG_SLOT, BooleanValue(enabled));
    }

    void setSticky(bool enabled) {
        setSlot(STICKY_FLAG_SLOT, BooleanValue(enabled));
    }

    bool ignoreCase() const { return getFixedSlot(IGNORE_CASE_FLAG_SLOT).toBoolean(); }
    bool global() const     { return getFixedSlot(GLOBAL_FLAG_SLOT).toBoolean(); }
    bool multiline() const  { return getFixedSlot(MULTILINE_FLAG_SLOT).toBoolean(); }
    bool sticky() const     { return getFixedSlot(STICKY_FLAG_SLOT).toBoolean(); }

    void shared(RegExpGuard *g) const {
        JS_ASSERT(maybeShared() != nullptr);
        g->init(*maybeShared());
    }

    bool getShared(ExclusiveContext *cx, RegExpGuard *g) {
        if (RegExpShared *shared = maybeShared()) {
            g->init(*shared);
            return true;
        }
        return createShared(cx, g);
    }

    void setShared(ExclusiveContext *cx, RegExpShared &shared) {
        shared.prepareForUse(cx);
        JSObject::setPrivate(&shared);
    }

  private:
    friend class RegExpObjectBuilder;

    
    friend bool
    EmptyShape::ensureInitialCustomShape<RegExpObject>(ExclusiveContext *cx,
                                                       Handle<RegExpObject*> obj);

    




    static Shape *
    assignInitialShape(ExclusiveContext *cx, Handle<RegExpObject*> obj);

    bool init(ExclusiveContext *cx, HandleAtom source, RegExpFlag flags);

    



    bool createShared(ExclusiveContext *cx, RegExpGuard *g);
    RegExpShared *maybeShared() const {
        return static_cast<RegExpShared *>(JSObject::getPrivate());
    }

    
    void setPrivate(void *priv) MOZ_DELETE;
};







bool
ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut);









inline bool
RegExpToShared(JSContext *cx, HandleObject obj, RegExpGuard *g)
{
    if (obj->is<RegExpObject>())
        return obj->as<RegExpObject>().getShared(cx, g);
    return Proxy::regexp_toShared(cx, obj, g);
}

template<XDRMode mode>
bool
XDRScriptRegExpObject(XDRState<mode> *xdr, HeapPtrObject *objp);

extern JSObject *
CloneScriptRegExpObject(JSContext *cx, RegExpObject &re);

} 

#endif 
