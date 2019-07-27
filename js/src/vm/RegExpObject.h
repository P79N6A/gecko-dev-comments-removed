





#ifndef vm_RegExpObject_h
#define vm_RegExpObject_h

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jsproxy.h"

#include "gc/Marking.h"
#include "gc/Zone.h"
#include "vm/Shape.h"



















namespace js {

struct MatchPair;
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
    explicit RegExpObjectBuilder(ExclusiveContext *cx, RegExpObject *reobj = nullptr);

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

    typedef frontend::TokenStream TokenStream;

    
    HeapPtrAtom        source;

    RegExpFlag         flags;
    size_t             parenCount;
    bool               canStringMatch;
    bool               marked_;

    HeapPtrJitCode     jitCodeLatin1;
    HeapPtrJitCode     jitCodeTwoByte;
    uint8_t            *byteCodeLatin1;
    uint8_t            *byteCodeTwoByte;

    
    Vector<uint8_t *, 0, SystemAllocPolicy> tables;

    
    bool compile(JSContext *cx, HandleLinearString input);
    bool compile(JSContext *cx, HandleAtom pattern, HandleLinearString input);

    bool compileIfNecessary(JSContext *cx, HandleLinearString input);

  public:
    RegExpShared(JSAtom *source, RegExpFlag flags);
    ~RegExpShared();

    
    RegExpRunStatus execute(JSContext *cx, HandleLinearString input, size_t *lastIndex,
                            MatchPairs &matches);

    
    bool addTable(uint8_t *table) {
        return tables.append(table);
    }

    

    size_t getParenCount() const {
        JS_ASSERT(isCompiled() || canStringMatch);
        return parenCount;
    }

    
    size_t pairCount() const            { return getParenCount() + 1; }

    JSAtom *getSource() const           { return source; }
    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const             { return flags & IgnoreCaseFlag; }
    bool global() const                 { return flags & GlobalFlag; }
    bool multiline() const              { return flags & MultilineFlag; }
    bool sticky() const                 { return flags & StickyFlag; }

    bool hasJitCodeLatin1() const {
        return jitCodeLatin1 != nullptr;
    }
    bool hasJitCodeTwoByte() const {
        return jitCodeTwoByte != nullptr;
    }
    bool hasByteCodeLatin1() const {
        return byteCodeLatin1 != nullptr;
    }
    bool hasByteCodeTwoByte() const {
        return byteCodeTwoByte != nullptr;
    }
    uint8_t *maybeByteCode(bool latin1) const {
        return latin1 ? byteCodeLatin1 : byteCodeTwoByte;
    }

    bool isCompiled(bool latin1) const {
        if (latin1)
            return hasJitCodeLatin1() || hasByteCodeLatin1();
        return hasJitCodeTwoByte() || hasByteCodeTwoByte();
    }
    bool isCompiled() const {
        return isCompiled(true) || isCompiled(false);
    }

    void trace(JSTracer *trc);

    bool marked() const { return marked_; }
    void clearMarked() { JS_ASSERT(marked_); marked_ = false; }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};





class RegExpGuard : public JS::CustomAutoRooter
{
    RegExpShared *re_;

    RegExpGuard(const RegExpGuard &) MOZ_DELETE;
    void operator=(const RegExpGuard &) MOZ_DELETE;

  public:
    explicit RegExpGuard(ExclusiveContext *cx)
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
    }

    void release() {
        re_ = nullptr;
    }

    virtual void trace(JSTracer *trc) {
        if (re_)
            re_->trace(trc);
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
        MOZ_IMPLICIT Key(RegExpShared *shared)
          : atom(shared->getSource()), flag(shared->getFlags())
        { }

        typedef Key Lookup;
        static HashNumber hash(const Lookup &l) {
            return DefaultHasher<JSAtom *>::hash(l.atom) ^ (l.flag << 1);
        }
        static bool match(Key l, Key r) {
            return l.atom == r.atom && l.flag == r.flag;
        }
    };

    



    typedef HashSet<RegExpShared *, Key, RuntimeAllocPolicy> Set;
    Set set_;

    




    ReadBarrieredObject matchResultTemplateObject_;

    JSObject *createMatchResultTemplateObject(JSContext *cx);

  public:
    explicit RegExpCompartment(JSRuntime *rt);
    ~RegExpCompartment();

    bool init(JSContext *cx);
    void sweep(JSRuntime *rt);

    bool empty() { return set_.empty(); }

    bool get(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g);

    
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
    create(ExclusiveContext *cx, RegExpStatics *res, const char16_t *chars, size_t length,
           RegExpFlag flags, frontend::TokenStream *ts, LifoAlloc &alloc);

    static RegExpObject *
    createNoStatics(ExclusiveContext *cx, const char16_t *chars, size_t length, RegExpFlag flags,
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

    bool needUpdateLastIndex() const {
        return sticky() || global();
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

    bool getShared(JSContext *cx, RegExpGuard *g);

    void setShared(RegExpShared &shared) {
        JS_ASSERT(!maybeShared());
        JSObject::setPrivate(&shared);
    }

    static void trace(JSTracer *trc, JSObject *obj);

  private:
    friend class RegExpObjectBuilder;

    
    friend bool
    EmptyShape::ensureInitialCustomShape<RegExpObject>(ExclusiveContext *cx,
                                                       Handle<RegExpObject*> obj);

    




    static Shape *
    assignInitialShape(ExclusiveContext *cx, Handle<RegExpObject*> obj);

    bool init(ExclusiveContext *cx, HandleAtom source, RegExpFlag flags);

    



    bool createShared(JSContext *cx, RegExpGuard *g);
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
