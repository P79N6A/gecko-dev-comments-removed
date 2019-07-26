






#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"

namespace js {





































class StaticScopeIter
{
    JSObject *obj;
    bool onNamedLambda;

  public:
    explicit StaticScopeIter(JSObject *obj);

    bool done() const;
    void operator++(int);

    
    bool hasDynamicScopeObject() const;
    Shape *scopeShape() const;

    enum Type { BLOCK, FUNCTION, NAMED_LAMBDA };
    Type type() const;

    StaticBlockObject &block() const;
    JSScript *funScript() const;
};












struct ScopeCoordinate
{
    uint16_t hops;
    uint16_t slot;

    inline ScopeCoordinate(jsbytecode *pc);
    inline ScopeCoordinate() {}
};





extern StaticScopeIter
ScopeCoordinateToStaticScope(JSScript *script, jsbytecode *pc);


extern PropertyName *
ScopeCoordinateName(JSRuntime *rt, JSScript *script, jsbytecode *pc);









































class ScopeObject : public JSObject
{
  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject &enclosingScope() const;
    inline void setEnclosingScope(HandleObject obj);

    





    inline const Value &aliasedVar(ScopeCoordinate sc);
    inline void setAliasedVar(ScopeCoordinate sc, const Value &v);

    
    static inline size_t offsetOfEnclosingScope();

    static inline size_t enclosingScopeSlot() {
        return SCOPE_CHAIN_SLOT;
    }
};

class CallObject : public ScopeObject
{
    static const uint32_t CALLEE_SLOT = 1;

    static CallObject *
    create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleFunction callee);

  public:
    
    static CallObject *
    create(JSContext *cx, HandleShape shape, HandleTypeObject type, HeapSlot *slots);

    static CallObject *
    createTemplateObject(JSContext *cx, JSScript *script);

    static const uint32_t RESERVED_SLOTS = 2;

    static CallObject *createForFunction(JSContext *cx, StackFrame *fp);
    static CallObject *createForStrictEval(JSContext *cx, StackFrame *fp);

    
    inline bool isForEval() const;

    



    inline JSFunction &callee() const;

    
    inline const Value &aliasedVar(AliasedFormalIter fi);
    inline void setAliasedVar(AliasedFormalIter fi, const Value &v);

    
    static inline size_t offsetOfCallee();
    static inline size_t calleeSlot() {
        return CALLEE_SLOT;
    }
};

class DeclEnvObject : public ScopeObject
{
  public:
    static const uint32_t RESERVED_SLOTS = 1;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT2;

    static DeclEnvObject *create(JSContext *cx, StackFrame *fp);
};

class NestedScopeObject : public ScopeObject
{
  protected:
    static const unsigned DEPTH_SLOT = 1;

  public:
    
    uint32_t stackDepth() const;
};

class WithObject : public NestedScopeObject
{
    static const unsigned THIS_SLOT = 2;

    
    JSObject *getProto() const;

  public:
    static const unsigned RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    static WithObject *
    create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth);

    
    JSObject &withThis() const;

    
    JSObject &object() const;
};

class BlockObject : public NestedScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    
    inline uint32_t slotCount() const;

    




    unsigned slotToLocalIndex(const Bindings &bindings, unsigned slot);
    unsigned localIndexToSlot(const Bindings &bindings, uint32_t i);

  protected:
    
    inline const Value &slotValue(unsigned i);
    inline void setSlotValue(unsigned i, const Value &v);
};

class StaticBlockObject : public BlockObject
{
  public:
    static StaticBlockObject *create(JSContext *cx);

    
    inline JSObject *enclosingStaticScope() const;

    



    inline StaticBlockObject *enclosingBlock() const;

    



    bool containsVarAtDepth(uint32_t depth);

    



    bool isAliased(unsigned i);

    



    bool needsClone();

    

    
    void setAliased(unsigned i, bool aliased);
    void setStackDepth(uint32_t depth);
    void initEnclosingStaticScope(JSObject *obj);

    



    void setDefinitionParseNode(unsigned i, frontend::Definition *def);
    frontend::Definition *maybeDefinitionParseNode(unsigned i);

    





    void initPrevBlockChainFromParser(StaticBlockObject *prev);
    void resetPrevBlockChainFromParser();

    static Shape *addVar(JSContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                         int index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, Handle<StaticBlockObject *> block,
                                     StackFrame *fp);

    
    StaticBlockObject &staticBlock() const;

    
    const Value &var(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    void setVar(unsigned i, const Value &v, MaybeCheckAliasing = CHECK_ALIASING);

    
    void copyUnaliasedValues(StackFrame *fp);
};

template<XDRMode mode>
bool
XDRStaticBlockObject(XDRState<mode> *xdr, HandleObject enclosingScope, HandleScript script,
                     StaticBlockObject **objp);

extern JSObject *
CloneStaticBlockObject(JSContext *cx, HandleObject enclosingScope, Handle<StaticBlockObject*> src);



class ScopeIterKey;














class ScopeIter
{
    friend class ScopeIterKey;

  public:
    enum Type { Call, Block, With, StrictEvalScope };

  private:
    StackFrame *fp_;
    RootedObject cur_;
    Rooted<StaticBlockObject *> block_;
    Type type_;
    bool hasScopeObject_;

    void settle();

    
    ScopeIter(const ScopeIter &si) MOZ_DELETE;

  public:
    
    explicit ScopeIter(JSContext *cx
                       JS_GUARD_OBJECT_NOTIFIER_PARAM);

    
    explicit ScopeIter(const ScopeIter &si, JSContext *cx
                       JS_GUARD_OBJECT_NOTIFIER_PARAM);

    
    explicit ScopeIter(StackFrame *fp, JSContext *cx
                       JS_GUARD_OBJECT_NOTIFIER_PARAM);

    



    explicit ScopeIter(JSObject &enclosingScope, JSContext *cx
                       JS_GUARD_OBJECT_NOTIFIER_PARAM);

    



    ScopeIter(const ScopeIter &si, StackFrame *fp, JSContext *cx
              JS_GUARD_OBJECT_NOTIFIER_PARAM);

    
    ScopeIter(StackFrame *fp, ScopeObject &scope, JSContext *cx
              JS_GUARD_OBJECT_NOTIFIER_PARAM);

    bool done() const { return !fp_; }

    

    JSObject &enclosingScope() const { JS_ASSERT(done()); return *cur_; }

    

    ScopeIter &operator++();

    StackFrame *fp() const { JS_ASSERT(!done()); return fp_; }
    Type type() const { JS_ASSERT(!done()); return type_; }
    bool hasScopeObject() const { JS_ASSERT(!done()); return hasScopeObject_; }
    ScopeObject &scope() const;

    StaticBlockObject &staticBlock() const { JS_ASSERT(type() == Block); return *block_; }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class ScopeIterKey
{
    StackFrame *fp_;
    JSObject *cur_;
    StaticBlockObject *block_;
    ScopeIter::Type type_;

  public:
    ScopeIterKey() : fp_(NULL), cur_(NULL), block_(NULL), type_() {}
    ScopeIterKey(const ScopeIter &si)
      : fp_(si.fp_), cur_(si.cur_), block_(si.block_), type_(si.type_)
    {}

    StackFrame *fp() const { return fp_; }
    ScopeIter::Type type() const { return type_; }

    
    typedef ScopeIterKey Lookup;
    static HashNumber hash(ScopeIterKey si);
    static bool match(ScopeIterKey si1, ScopeIterKey si2);
};




























extern JSObject *
GetDebugScopeForFunction(JSContext *cx, JSFunction *fun);

extern JSObject *
GetDebugScopeForFrame(JSContext *cx, StackFrame *fp);


class DebugScopeObject : public JSObject
{
    



    static const unsigned ENCLOSING_EXTRA = 0;

    



    static const unsigned SNAPSHOT_EXTRA = 1;

  public:
    static DebugScopeObject *create(JSContext *cx, ScopeObject &scope, HandleObject enclosing);

    ScopeObject &scope() const;
    JSObject &enclosingScope() const;

    
    JSObject *maybeSnapshot() const;
    void initSnapshot(JSObject &snapshot);

    
    bool isForDeclarative() const;
};


class DebugScopes
{
    JSRuntime *rt;

    
    typedef WeakMap<HeapPtrObject, HeapPtrObject> ObjectWeakMap;
    ObjectWeakMap proxiedScopes;

    



    typedef HashMap<ScopeIterKey,
                    ReadBarriered<DebugScopeObject>,
                    ScopeIterKey,
                    RuntimeAllocPolicy> MissingScopeMap;
    MissingScopeMap missingScopes;

    






    typedef HashMap<ScopeObject *,
                    StackFrame *,
                    DefaultHasher<ScopeObject *>,
                    RuntimeAllocPolicy> LiveScopeMap;
    LiveScopeMap liveScopes;

  public:
    DebugScopes(JSRuntime *rt);
    ~DebugScopes();
    bool init();

    void mark(JSTracer *trc);
    void sweep();

    DebugScopeObject *hasDebugScope(JSContext *cx, ScopeObject &scope) const;
    bool addDebugScope(JSContext *cx, ScopeObject &scope, DebugScopeObject &debugScope);

    DebugScopeObject *hasDebugScope(JSContext *cx, const ScopeIter &si) const;
    bool addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope);

    bool updateLiveScopes(JSContext *cx);
    StackFrame *hasLiveFrame(ScopeObject &scope);

    



    void onPopCall(StackFrame *fp, JSContext *cx);
    void onPopBlock(JSContext *cx, StackFrame *fp);
    void onPopWith(StackFrame *fp);
    void onPopStrictEvalScope(StackFrame *fp);
    void onGeneratorFrameChange(StackFrame *from, StackFrame *to, JSContext *cx);
    void onCompartmentLeaveDebugMode(JSCompartment *c);
};

}  
#endif
