





#ifndef vm_ScopeObject_h
#define vm_ScopeObject_h

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"
#include "vm/ProxyObject.h"

namespace js {

namespace frontend { struct Definition; }





































class StaticScopeIter
{
    RootedObject obj;
    bool onNamedLambda;

  public:
    explicit StaticScopeIter(ExclusiveContext *cx, JSObject *obj);

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

    inline ScopeCoordinate(jsbytecode *pc)
      : hops(GET_UINT16(pc)), slot(GET_UINT16(pc + 2))
    {
        JS_ASSERT(JOF_OPTYPE(*pc) == JOF_SCOPECOORD);
    }

    inline ScopeCoordinate() {}
};





extern Shape *
ScopeCoordinateToStaticScopeShape(JSContext *cx, JSScript *script, jsbytecode *pc);


extern PropertyName *
ScopeCoordinateName(JSContext *cx, JSScript *script, jsbytecode *pc);


extern JSScript *
ScopeCoordinateFunctionScript(JSContext *cx, JSScript *script, jsbytecode *pc);









































class ScopeObject : public JSObject
{
  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject &enclosingScope() const {
        return getReservedSlot(SCOPE_CHAIN_SLOT).toObject();
    }

    inline void setEnclosingScope(HandleObject obj);

    





    inline const Value &aliasedVar(ScopeCoordinate sc);

    inline void setAliasedVar(JSContext *cx, ScopeCoordinate sc, PropertyName *name, const Value &v);

    
    static size_t offsetOfEnclosingScope() {
        return getFixedSlotOffset(SCOPE_CHAIN_SLOT);
    }

    static size_t enclosingScopeSlot() {
        return SCOPE_CHAIN_SLOT;
    }
};

class CallObject : public ScopeObject
{
    static const uint32_t CALLEE_SLOT = 1;

    static CallObject *
    create(JSContext *cx, HandleScript script, HandleObject enclosing, HandleFunction callee);

  public:
    static const Class class_;

    
    static CallObject *
    create(JSContext *cx, HandleScript script, HandleShape shape, HandleTypeObject type, HeapSlot *slots);

    static CallObject *
    createTemplateObject(JSContext *cx, HandleScript script, gc::InitialHeap heap);

    static const uint32_t RESERVED_SLOTS = 2;

    static CallObject *createForFunction(JSContext *cx, HandleObject enclosing, HandleFunction callee);

    static CallObject *createForFunction(JSContext *cx, AbstractFramePtr frame);
    static CallObject *createForStrictEval(JSContext *cx, AbstractFramePtr frame);

    
    bool isForEval() const {
        JS_ASSERT(getReservedSlot(CALLEE_SLOT).isObjectOrNull());
        JS_ASSERT_IF(getReservedSlot(CALLEE_SLOT).isObject(),
                     getReservedSlot(CALLEE_SLOT).toObject().is<JSFunction>());
        return getReservedSlot(CALLEE_SLOT).isNull();
    }

    



    JSFunction &callee() const {
        return getReservedSlot(CALLEE_SLOT).toObject().as<JSFunction>();
    }

    
    const Value &aliasedVar(AliasedFormalIter fi) {
        return getSlot(fi.scopeSlot());
    }

    inline void setAliasedVar(JSContext *cx, AliasedFormalIter fi, PropertyName *name, const Value &v);

    
    static size_t offsetOfCallee() {
        return getFixedSlotOffset(CALLEE_SLOT);
    }

    static size_t calleeSlot() {
        return CALLEE_SLOT;
    }
};

class DeclEnvObject : public ScopeObject
{
    
    static const uint32_t LAMBDA_SLOT = 1;

  public:
    static const uint32_t RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT2_BACKGROUND;

    static const Class class_;

    static DeclEnvObject *
    createTemplateObject(JSContext *cx, HandleFunction fun, gc::InitialHeap heap);

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
    
    uint32_t stackDepth() const {
        return getReservedSlot(DEPTH_SLOT).toPrivateUint32();
    }
};

class WithObject : public NestedScopeObject
{
    static const unsigned THIS_SLOT = 2;

    
    JSObject *getProto() const;

  public:
    static const unsigned RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    static const Class class_;

    static WithObject *
    create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth);

    
    JSObject &withThis() const {
        return getReservedSlot(THIS_SLOT).toObject();
    }

    
    JSObject &object() const {
        return *JSObject::getProto();
    }
};

class BlockObject : public NestedScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    static const Class class_;

    
    uint32_t slotCount() const {
        return propertyCount();
    }

    




    unsigned slotToLocalIndex(const Bindings &bindings, unsigned slot) {
        JS_ASSERT(slot < RESERVED_SLOTS + slotCount());
        return bindings.numVars() + stackDepth() + (slot - RESERVED_SLOTS);
    }

    unsigned localIndexToSlot(const Bindings &bindings, uint32_t i) {
        return RESERVED_SLOTS + (i - (bindings.numVars() + stackDepth()));
    }

  protected:
    
    const Value &slotValue(unsigned i) {
        return getSlotRef(RESERVED_SLOTS + i);
    }

    void setSlotValue(unsigned i, const Value &v) {
        setSlot(RESERVED_SLOTS + i, v);
    }
};

class StaticBlockObject : public BlockObject
{
  public:
    static StaticBlockObject *create(ExclusiveContext *cx);

    
    JSObject *enclosingStaticScope() const {
        return getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    



    inline StaticBlockObject *enclosingBlock() const;

    



    bool containsVarAtDepth(uint32_t depth) {
        return depth >= stackDepth() && depth < stackDepth() + slotCount();
    }

    



    bool isAliased(unsigned i) {
        return slotValue(i).isTrue();
    }

    



    bool needsClone() {
        return !slotValue(0).isFalse();
    }

    

    
    void setAliased(unsigned i, bool aliased) {
        JS_ASSERT_IF(i > 0, slotValue(i-1).isBoolean());
        setSlotValue(i, BooleanValue(aliased));
        if (aliased && !needsClone()) {
            setSlotValue(0, MagicValue(JS_BLOCK_NEEDS_CLONE));
            JS_ASSERT(needsClone());
        }
    }

    void setStackDepth(uint32_t depth) {
        JS_ASSERT(getReservedSlot(DEPTH_SLOT).isUndefined());
        initReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));
    }

    void initEnclosingStaticScope(JSObject *obj) {
        JS_ASSERT(getReservedSlot(SCOPE_CHAIN_SLOT).isUndefined());
        setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(obj));
    }

    



    void setDefinitionParseNode(unsigned i, frontend::Definition *def) {
        JS_ASSERT(slotValue(i).isUndefined());
        setSlotValue(i, PrivateValue(def));
    }

    frontend::Definition *maybeDefinitionParseNode(unsigned i) {
        Value v = slotValue(i);
        return v.isUndefined() ? nullptr
                               : reinterpret_cast<frontend::Definition *>(v.toPrivate());
    }

    





    void initPrevBlockChainFromParser(StaticBlockObject *prev) {
        setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(prev));
    }

    void resetPrevBlockChainFromParser() {
        setReservedSlot(SCOPE_CHAIN_SLOT, UndefinedValue());
    }

    static Shape *addVar(ExclusiveContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                         int index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, Handle<StaticBlockObject *> block,
                                     AbstractFramePtr frame);

    
    StaticBlockObject &staticBlock() const {
        return getProto()->as<StaticBlockObject>();
    }

    
    const Value &var(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT_IF(checkAliasing, staticBlock().isAliased(i));
        return slotValue(i);
    }

    void setVar(unsigned i, const Value &v, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT_IF(checkAliasing, staticBlock().isAliased(i));
        setSlotValue(i, v);
    }

    
    void copyUnaliasedValues(AbstractFramePtr frame);
};

template<XDRMode mode>
bool
XDRStaticBlockObject(XDRState<mode> *xdr, HandleObject enclosingScope,
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

    ScopeIter(JSContext *cx) MOZ_DELETE;

  public:

    
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
    ScopeIterKey() : frame_(NullFramePtr()), cur_(nullptr), block_(nullptr), type_() {}
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


class DebugScopeObject : public ObjectProxyObject
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

template<>
inline bool
JSObject::is<js::NestedScopeObject>() const
{
    return is<js::BlockObject>() || is<js::WithObject>();
}

template<>
inline bool
JSObject::is<js::ScopeObject>() const
{
    return is<js::CallObject>() || is<js::DeclEnvObject>() || is<js::NestedScopeObject>();
}

template<>
inline bool
JSObject::is<js::DebugScopeObject>() const
{
    extern bool js_IsDebugScopeSlow(js::ObjectProxyObject *proxy);

    
    return hasClass(&js::ObjectProxyObject::class_) &&
           js_IsDebugScopeSlow(&const_cast<JSObject*>(this)->as<js::ObjectProxyObject>());
}

template<>
inline bool
JSObject::is<js::ClonedBlockObject>() const
{
    return is<js::BlockObject>() && !!getProto();
}

template<>
inline bool
JSObject::is<js::StaticBlockObject>() const
{
    return is<js::BlockObject>() && !getProto();
}

inline JSObject *
JSObject::enclosingScope()
{
    return is<js::ScopeObject>()
           ? &as<js::ScopeObject>().enclosingScope()
           : is<js::DebugScopeObject>()
           ? &as<js::DebugScopeObject>().enclosingScope()
           : getParent();
}

namespace js {

inline const Value &
ScopeObject::aliasedVar(ScopeCoordinate sc)
{
    JS_ASSERT(is<CallObject>() || is<ClonedBlockObject>());
    return getSlot(sc.slot);
}

inline StaticBlockObject *
StaticBlockObject::enclosingBlock() const
{
    JSObject *obj = getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    return obj && obj->is<StaticBlockObject>() ? &obj->as<StaticBlockObject>() : nullptr;
}

#ifdef DEBUG
bool
AnalyzeEntrainedVariables(JSContext *cx, HandleScript script);
#endif

} 

#endif
