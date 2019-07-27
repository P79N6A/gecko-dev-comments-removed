





#ifndef vm_RegExpObject_h
#define vm_RegExpObject_h

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"

#include "gc/Marking.h"
#include "gc/Zone.h"
#include "proxy/Proxy.h"
#include "vm/ArrayObject.h"
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
    ExclusiveContext* cx;
    Rooted<RegExpObject*> reobj_;

    bool getOrCreate();
    bool getOrCreateClone(HandleObjectGroup group);

  public:
    explicit RegExpObjectBuilder(ExclusiveContext* cx, RegExpObject* reobj = nullptr);

    RegExpObject* reobj() { return reobj_; }

    RegExpObject* build(HandleAtom source, RegExpFlag flags);
    RegExpObject* build(HandleAtom source, RegExpShared& shared);

    
    RegExpObject* clone(Handle<RegExpObject*> other);
};

JSObject*
CloneRegExpObject(JSContext* cx, JSObject* obj);














class RegExpShared
{
  public:
    enum CompilationMode {
        Normal,
        MatchOnly
    };

    enum ForceByteCodeEnum {
        DontForceByteCode,
        ForceByteCode
    };

  private:
    friend class RegExpCompartment;
    friend class RegExpStatics;

    typedef frontend::TokenStream TokenStream;

    struct RegExpCompilation
    {
        RelocatablePtrJitCode jitCode;
        uint8_t* byteCode;

        RegExpCompilation() : byteCode(nullptr) {}
        ~RegExpCompilation() { js_free(byteCode); }

        bool compiled(ForceByteCodeEnum force = DontForceByteCode) const {
            return byteCode || (force == DontForceByteCode && jitCode);
        }
    };

    
    RelocatablePtrAtom source;

    RegExpFlag         flags;
    size_t             parenCount;
    bool               canStringMatch;
    bool               marked_;

    RegExpCompilation  compilationArray[4];

    static int CompilationIndex(CompilationMode mode, bool latin1) {
        switch (mode) {
          case Normal:    return latin1 ? 0 : 1;
          case MatchOnly: return latin1 ? 2 : 3;
        }
        MOZ_CRASH();
    }

    
    Vector<uint8_t*, 0, SystemAllocPolicy> tables;

    
    bool compile(JSContext* cx, HandleLinearString input,
                 CompilationMode mode, ForceByteCodeEnum force);
    bool compile(JSContext* cx, HandleAtom pattern, HandleLinearString input,
                 CompilationMode mode, ForceByteCodeEnum force);

    bool compileIfNecessary(JSContext* cx, HandleLinearString input,
                            CompilationMode mode, ForceByteCodeEnum force);

    const RegExpCompilation& compilation(CompilationMode mode, bool latin1) const {
        return compilationArray[CompilationIndex(mode, latin1)];
    }

    RegExpCompilation& compilation(CompilationMode mode, bool latin1) {
        return compilationArray[CompilationIndex(mode, latin1)];
    }

  public:
    RegExpShared(JSAtom* source, RegExpFlag flags);
    ~RegExpShared();

    
    
    RegExpRunStatus execute(JSContext* cx, HandleLinearString input, size_t searchIndex,
                            MatchPairs* matches);

    
    bool addTable(uint8_t* table) {
        return tables.append(table);
    }

    

    size_t getParenCount() const {
        MOZ_ASSERT(isCompiled());
        return parenCount;
    }

    
    size_t pairCount() const            { return getParenCount() + 1; }

    JSAtom* getSource() const           { return source; }
    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const             { return flags & IgnoreCaseFlag; }
    bool global() const                 { return flags & GlobalFlag; }
    bool multiline() const              { return flags & MultilineFlag; }
    bool sticky() const                 { return flags & StickyFlag; }

    bool isCompiled(CompilationMode mode, bool latin1,
                    ForceByteCodeEnum force = DontForceByteCode) const {
        return compilation(mode, latin1).compiled(force);
    }
    bool isCompiled() const {
        return isCompiled(Normal, true) || isCompiled(Normal, false)
            || isCompiled(MatchOnly, true) || isCompiled(MatchOnly, false);
    }

    void trace(JSTracer* trc);

    bool marked() const { return marked_; }
    void clearMarked() { marked_ = false; }

    static size_t offsetOfSource() {
        return offsetof(RegExpShared, source);
    }

    static size_t offsetOfFlags() {
        return offsetof(RegExpShared, flags);
    }

    static size_t offsetOfParenCount() {
        return offsetof(RegExpShared, parenCount);
    }

    static size_t offsetOfJitCode(CompilationMode mode, bool latin1) {
        return offsetof(RegExpShared, compilationArray)
             + (CompilationIndex(mode, latin1) * sizeof(RegExpCompilation))
             + offsetof(RegExpCompilation, jitCode);
    }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};





class RegExpGuard : public JS::CustomAutoRooter
{
    RegExpShared* re_;

    RegExpGuard(const RegExpGuard&) = delete;
    void operator=(const RegExpGuard&) = delete;

  public:
    explicit RegExpGuard(ExclusiveContext* cx)
      : CustomAutoRooter(cx), re_(nullptr)
    {}

    RegExpGuard(ExclusiveContext* cx, RegExpShared& re)
      : CustomAutoRooter(cx), re_(nullptr)
    {
        init(re);
    }

    ~RegExpGuard() {
        release();
    }

  public:
    void init(RegExpShared& re) {
        MOZ_ASSERT(!initialized());
        re_ = &re;
    }

    void release() {
        re_ = nullptr;
    }

    virtual void trace(JSTracer* trc) {
        if (re_)
            re_->trace(trc);
    }

    bool initialized() const { return !!re_; }
    RegExpShared* re() const { MOZ_ASSERT(initialized()); return re_; }
    RegExpShared* operator->() { return re(); }
    RegExpShared& operator*() { return *re(); }
};

class RegExpCompartment
{
    struct Key {
        JSAtom* atom;
        uint16_t flag;

        Key() {}
        Key(JSAtom* atom, RegExpFlag flag)
          : atom(atom), flag(flag)
        { }
        MOZ_IMPLICIT Key(RegExpShared* shared)
          : atom(shared->getSource()), flag(shared->getFlags())
        { }

        typedef Key Lookup;
        static HashNumber hash(const Lookup& l) {
            return DefaultHasher<JSAtom*>::hash(l.atom) ^ (l.flag << 1);
        }
        static bool match(Key l, Key r) {
            return l.atom == r.atom && l.flag == r.flag;
        }
    };

    



    typedef HashSet<RegExpShared*, Key, RuntimeAllocPolicy> Set;
    Set set_;

    




    ReadBarriered<ArrayObject*> matchResultTemplateObject_;

    ArrayObject* createMatchResultTemplateObject(JSContext* cx);

  public:
    explicit RegExpCompartment(JSRuntime* rt);
    ~RegExpCompartment();

    bool init(JSContext* cx);
    void sweep(JSRuntime* rt);

    bool empty() { return set_.empty(); }

    bool get(JSContext* cx, JSAtom* source, RegExpFlag flags, RegExpGuard* g);

    
    bool get(JSContext* cx, HandleAtom source, JSString* maybeOpt, RegExpGuard* g);

    
    ArrayObject* getOrCreateMatchResultTemplateObject(JSContext* cx) {
        if (matchResultTemplateObject_)
            return matchResultTemplateObject_;
        return createMatchResultTemplateObject(cx);
    }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

class RegExpObject : public NativeObject
{
    static const unsigned LAST_INDEX_SLOT          = 0;
    static const unsigned SOURCE_SLOT              = 1;
    static const unsigned GLOBAL_FLAG_SLOT         = 2;
    static const unsigned IGNORE_CASE_FLAG_SLOT    = 3;
    static const unsigned MULTILINE_FLAG_SLOT      = 4;
    static const unsigned STICKY_FLAG_SLOT         = 5;

  public:
    static const unsigned RESERVED_SLOTS = 6;
    static const unsigned PRIVATE_SLOT = 7;

    static const Class class_;

    




    static RegExpObject*
    create(ExclusiveContext* cx, RegExpStatics* res, const char16_t* chars, size_t length,
           RegExpFlag flags, frontend::TokenStream* ts, LifoAlloc& alloc);

    static RegExpObject*
    createNoStatics(ExclusiveContext* cx, const char16_t* chars, size_t length, RegExpFlag flags,
                    frontend::TokenStream* ts, LifoAlloc& alloc);

    static RegExpObject*
    createNoStatics(ExclusiveContext* cx, HandleAtom atom, RegExpFlag flags,
                    frontend::TokenStream* ts, LifoAlloc& alloc);

    




    static Shape*
    assignInitialShape(ExclusiveContext* cx, Handle<RegExpObject*> obj);

    

    static unsigned lastIndexSlot() { return LAST_INDEX_SLOT; }

    const Value& getLastIndex() const { return getSlot(LAST_INDEX_SLOT); }

    void setLastIndex(double d) {
        setSlot(LAST_INDEX_SLOT, NumberValue(d));
    }

    void zeroLastIndex() {
        setSlot(LAST_INDEX_SLOT, Int32Value(0));
    }

    JSFlatString* toString(JSContext* cx) const;

    JSAtom* getSource() const { return &getSlot(SOURCE_SLOT).toString()->asAtom(); }

    void setSource(JSAtom* source) {
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

    bool getShared(JSContext* cx, RegExpGuard* g);

    void setShared(RegExpShared& shared) {
        MOZ_ASSERT(!maybeShared());
        NativeObject::setPrivate(&shared);
    }

    static void trace(JSTracer* trc, JSObject* obj);

  private:
    friend class RegExpObjectBuilder;

    bool init(ExclusiveContext* cx, HandleAtom source, RegExpFlag flags);

    



    bool createShared(JSContext* cx, RegExpGuard* g);
    RegExpShared* maybeShared() const {
        return static_cast<RegExpShared*>(NativeObject::getPrivate(PRIVATE_SLOT));
    }

    
    void setPrivate(void* priv) = delete;
};







bool
ParseRegExpFlags(JSContext* cx, JSString* flagStr, RegExpFlag* flagsOut);


inline bool
RegExpToShared(JSContext* cx, HandleObject obj, RegExpGuard* g)
{
    if (obj->is<RegExpObject>())
        return obj->as<RegExpObject>().getShared(cx, g);
    MOZ_ASSERT(Proxy::objectClassIs(obj, ESClass_RegExp, cx));
    return Proxy::regexp_toShared(cx, obj, g);
}

template<XDRMode mode>
bool
XDRScriptRegExpObject(XDRState<mode>* xdr, MutableHandle<RegExpObject*> objp);

extern JSObject*
CloneScriptRegExpObject(JSContext* cx, RegExpObject& re);

JSAtom*
EscapeRegExpPattern(JSContext* cx, HandleAtom src);

} 

#endif 
