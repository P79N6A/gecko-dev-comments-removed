






#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "mozilla/GuardObjects.h"

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"

namespace js {





































class StaticScopeIter
{
    RootedObject obj;
    bool onNamedLambda;

  public:
    explicit StaticScopeIter(JSContext *cx, HandleObject obj);

    bool done() const;
    void operator++(int);

    
    bool hasDynamicScopeObject() const;
    RawShape scopeShape() const;

    enum Type { BLOCK, FUNCTION, NAMED_LAMBDA };
    Type type() const;

    StaticBlockObject &block() const;
    RawScript funScript() const;
};












struct ScopeCoordinate
{
    uint16_t hops;
    uint16_t slot;

    inline ScopeCoordinate(jsbytecode *pc);
    inline ScopeCoordinate() {}
};





extern RawShape
ScopeCoordinateToStaticScopeShape(JSContext *cx, JSScript *script, jsbytecode *pc);


extern PropertyName *
ScopeCoordinateName(JSContext *cx, JSScript *script, jsbytecode *pc);









































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
    create(JSContext *cx, HandleScript script, HandleObject enclosing, HandleFunction callee);

  public:
    
    static CallObject *
    create(JSContext *cx, HandleShape shape, HandleTypeObject type, HeapSlot *slots);

    static CallObject *
    createTemplateObject(JSContext *cx, HandleScript script);

    static const uint32_t RESERVED_SLOTS = 2;

    static CallObject *createForFunction(JSContext *cx, HandleObject enclosing, HandleFunction callee);

    static CallObject *createForFunction(JSContext *cx, AbstractFramePtr frame);
    static CallObject *createForStrictEval(JSContext *cx, AbstractFramePtr frame);

    
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
    
    static const uint32_t LAMBDA_SLOT = 1;

  public:
    static const uint32_t RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT2;

    static DeclEnvObject *
    createTemplateObject(JSContext *cx, HandleFunction fun);

    static DeclEnvObject *create(JSContext *cx, HandleObject enclosing, HandleFunction callee);

    static inline size_t lambdaSlot() {
        return LAMBDA_SLOT;
    }
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

    static RawShape addVar(JSContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                           int index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, Handle<StaticBlockObject *> block,
                                     AbstractFramePtr frame);

    
    StaticBlockObject &staticBlock() const;

    
    const Value &var(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    void setVar(unsigned i, const Value &v, MaybeCheckAliasing = CHECK_ALIASING);

    
    void copyUnaliasedValues(AbstractFramePtr frame);
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
    JSContext *cx;
    AbstractFramePtr frame_;
    RootedObject cur_;
    Rooted<StaticBlockObject *> block_;
    Type type_;
    bool hasScopeObject_;

    void settle();

    
    ScopeIter(const ScopeIter &si) MOZ_DELETE;

  public:
    
    explicit ScopeIter(JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    explicit ScopeIter(const ScopeIter &si, JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    explicit ScopeIter(AbstractFramePtr frame, JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    



    explicit ScopeIter(JSObject &enclosingScope, JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    



    ScopeIter(const ScopeIter &si, AbstractFramePtr frame, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    ScopeIter(AbstractFramePtr frame, ScopeObject &scope, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    bool done() const { return !frame_; }

    

    JSObject &enclosingScope() const { JS_ASSERT(done()); return *cur_; }

    

    ScopeIter &operator++();

    AbstractFramePtr frame() const { JS_ASSERT(!done()); return frame_; }
    Type type() const { JS_ASSERT(!done()); return type_; }
    bool hasScopeObject() const { JS_ASSERT(!done()); return hasScopeObject_; }
    ScopeObject &scope() const;

    StaticBlockObject &staticBlock() const { JS_ASSERT(type() == Block); return *block_; }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class ScopeIterKey
{
    AbstractFramePtr frame_;
    JSObject *cur_;
    StaticBlockObject *block_;
    ScopeIter::Type type_;

  public:
    ScopeIterKey() : frame_(NullFramePtr()), cur_(NULL), block_(NULL), type_() {}
    ScopeIterKey(const ScopeIter &si)
      : frame_(si.frame_), cur_(si.cur_), block_(si.block_), type_(si.type_)
    {}

    AbstractFramePtr frame() const { return frame_; }
    ScopeIter::Type type() const { return type_; }

    
    typedef ScopeIterKey Lookup;
    static HashNumber hash(ScopeIterKey si);
    static bool match(ScopeIterKey si1, ScopeIterKey si2);
};




























extern JSObject *
GetDebugScopeForFunction(JSContext *cx, HandleFunction fun);

extern JSObject *
GetDebugScopeForFrame(JSContext *cx, AbstractFramePtr frame);


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
    
    typedef WeakMap<EncapsulatedPtrObject, RelocatablePtrObject> ObjectWeakMap;
    ObjectWeakMap proxiedScopes;

    



    typedef HashMap<ScopeIterKey,
                    ReadBarriered<DebugScopeObject>,
                    ScopeIterKey,
                    RuntimeAllocPolicy> MissingScopeMap;
    MissingScopeMap missingScopes;

    






    typedef HashMap<ScopeObject *,
                    AbstractFramePtr,
                    DefaultHasher<ScopeObject *>,
                    RuntimeAllocPolicy> LiveScopeMap;
    LiveScopeMap liveScopes;

  public:
    DebugScopes(JSContext *c);
    ~DebugScopes();

  private:
    bool init();

    static DebugScopes *ensureCompartmentData(JSContext *cx);

  public:
    void mark(JSTracer *trc);
    void sweep(JSRuntime *rt);

    static DebugScopeObject *hasDebugScope(JSContext *cx, ScopeObject &scope);
    static bool addDebugScope(JSContext *cx, ScopeObject &scope, DebugScopeObject &debugScope);

    static DebugScopeObject *hasDebugScope(JSContext *cx, const ScopeIter &si);
    static bool addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope);

    static bool updateLiveScopes(JSContext *cx);
    static AbstractFramePtr hasLiveFrame(ScopeObject &scope);

    



    static void onPopCall(AbstractFramePtr frame, JSContext *cx);
    static void onPopBlock(JSContext *cx, AbstractFramePtr frame);
    static void onPopWith(AbstractFramePtr frame);
    static void onPopStrictEvalScope(AbstractFramePtr frame);
    static void onGeneratorFrameChange(AbstractFramePtr from, AbstractFramePtr to, JSContext *cx);
    static void onCompartmentLeaveDebugMode(JSCompartment *c);
};

}  
#endif
