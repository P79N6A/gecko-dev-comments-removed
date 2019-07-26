






#ifndef RegExpObject_h__
#define RegExpObject_h__

#include "mozilla/Attributes.h"

#include <stddef.h>
#include "jscntxt.h"
#include "jsobj.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/TemplateLib.h"
#include "vm/MatchPairs.h"

#include "yarr/MatchResult.h"
#include "yarr/Yarr.h"
#if ENABLE_YARR_JIT
#include "yarr/YarrJIT.h"
#endif
#include "yarr/YarrSyntaxChecker.h"



















namespace js {

enum RegExpRunStatus
{
    RegExpRunStatus_Error,
    RegExpRunStatus_Success,
    RegExpRunStatus_Success_NotFound
};

class RegExpObjectBuilder
{
    JSContext             *cx;
    Rooted<RegExpObject*> reobj_;

    bool getOrCreate();
    bool getOrCreateClone(RegExpObject *proto);

  public:
    RegExpObjectBuilder(JSContext *cx, RegExpObject *reobj = NULL);

    RegExpObject *reobj() { return reobj_; }

    RegExpObject *build(HandleAtom source, RegExpFlag flags);
    RegExpObject *build(HandleAtom source, RegExpShared &shared);

    
    RegExpObject *clone(Handle<RegExpObject*> other, Handle<RegExpObject*> proto);
};

JSObject *
CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);































class RegExpShared
{
    friend class RegExpCompartment;
    friend class RegExpStatics;
    friend class RegExpGuard;

    typedef frontend::TokenStream TokenStream;
    typedef JSC::Yarr::BytecodePattern BytecodePattern;
    typedef JSC::Yarr::ErrorCode ErrorCode;
    typedef JSC::Yarr::YarrPattern YarrPattern;
#if ENABLE_YARR_JIT
    typedef JSC::Yarr::JSGlobalData JSGlobalData;
    typedef JSC::Yarr::YarrCodeBlock YarrCodeBlock;
    typedef JSC::Yarr::YarrJITCompileMode YarrJITCompileMode;
#endif

    




    JSAtom *           source;

    RegExpFlag         flags;
    unsigned           parenCount;

#if ENABLE_YARR_JIT
    
    YarrCodeBlock   codeBlock;
#endif
    BytecodePattern *bytecode;

    
    size_t             activeUseCount;
    uint64_t           gcNumberWhenUsed;

    
    bool compile(JSContext *cx, bool matchOnly);
    bool compile(JSContext *cx, JSLinearString &pattern, bool matchOnly);

    bool compileIfNecessary(JSContext *cx);
    bool compileMatchOnlyIfNecessary(JSContext *cx);

  public:
    RegExpShared(JSRuntime *rt, JSAtom *source, RegExpFlag flags);
    ~RegExpShared();

    
    void trace(JSTracer *trc) {
        MarkStringUnbarriered(trc, &source, "regexpshared source");
    }

    
    static inline bool isJITRuntimeEnabled(JSContext *cx);
    static void reportYarrError(JSContext *cx, TokenStream *ts, ErrorCode error);
    static bool checkSyntax(JSContext *cx, TokenStream *tokenStream, JSLinearString *source);

    
    inline void prepareForUse(JSContext *cx);

    
    RegExpRunStatus execute(JSContext *cx, const jschar *chars, size_t length,
                            size_t *lastIndex, MatchPairs &matches);

    
    RegExpRunStatus executeMatchOnly(JSContext *cx, const jschar *chars, size_t length,
                                     size_t *lastIndex, MatchPair &match);

    

    size_t getParenCount() const        { JS_ASSERT(isCompiled()); return parenCount; }
    void incRef()                       { activeUseCount++; }
    void decRef()                       { JS_ASSERT(activeUseCount > 0); activeUseCount--; }

    
    size_t pairCount() const            { return getParenCount() + 1; }

    RegExpFlag getFlags() const         { return flags; }
    bool ignoreCase() const             { return flags & IgnoreCaseFlag; }
    bool global() const                 { return flags & GlobalFlag; }
    bool multiline() const              { return flags & MultilineFlag; }
    bool sticky() const                 { return flags & StickyFlag; }

#ifdef ENABLE_YARR_JIT
    bool hasCode() const                { return codeBlock.has16BitCode(); }
    bool hasMatchOnlyCode() const       { return codeBlock.has16BitCodeMatchOnly(); }
#else
    bool hasCode() const                { return false; }
    bool hasMatchOnlyCode() const       { return false; }
#endif
    bool hasBytecode() const            { return bytecode != NULL; }
    bool isCompiled() const             { return hasBytecode() || hasCode() || hasMatchOnlyCode(); }
};





class RegExpGuard
{
    RegExpShared *re_;

    




    RootedAtom source_;

    RegExpGuard(const RegExpGuard &) MOZ_DELETE;
    void operator=(const RegExpGuard &) MOZ_DELETE;

  public:
    inline RegExpGuard(JSContext *cx);
    inline RegExpGuard(JSContext *cx, RegExpShared &re);
    inline ~RegExpGuard();

  public:
    inline void init(RegExpShared &re);
    inline void release();

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

  public:
    RegExpCompartment(JSRuntime *rt);
    ~RegExpCompartment();

    bool init(JSContext *cx);
    void sweep(JSRuntime *rt);

    bool get(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g);

    
    bool get(JSContext *cx, HandleAtom source, JSString *maybeOpt, RegExpGuard *g);

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf);
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

    




    static RegExpObject *
    create(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
           RegExpFlag flags, frontend::TokenStream *ts);

    static RegExpObject *
    createNoStatics(JSContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                    frontend::TokenStream *ts);

    static RegExpObject *
    createNoStatics(JSContext *cx, HandleAtom atom, RegExpFlag flags, frontend::TokenStream *ts);

    

    static unsigned lastIndexSlot() { return LAST_INDEX_SLOT; }

    const Value &getLastIndex() const { return getSlot(LAST_INDEX_SLOT); }
    inline void setLastIndex(double d);
    inline void zeroLastIndex();

    JSFlatString *toString(JSContext *cx) const;

    JSAtom *getSource() const { return &getSlot(SOURCE_SLOT).toString()->asAtom(); }
    inline void setSource(JSAtom *source);

    RegExpFlag getFlags() const {
        unsigned flags = 0;
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

    




    RawShape assignInitialShape(JSContext *cx);

    bool init(JSContext *cx, HandleAtom source, RegExpFlag flags);

    



    bool createShared(JSContext *cx, RegExpGuard *g);
    RegExpShared *maybeShared() const;

    
    void setPrivate(void *priv) MOZ_DELETE;
};







bool
ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut);









inline bool
RegExpToShared(JSContext *cx, JSObject &obj, RegExpGuard *g);

template<XDRMode mode>
bool
XDRScriptRegExpObject(XDRState<mode> *xdr, HeapPtrObject *objp);

extern JSObject *
CloneScriptRegExpObject(JSContext *cx, RegExpObject &re);

} 

#endif
