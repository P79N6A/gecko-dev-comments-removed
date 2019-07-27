





#ifndef vm_ScopeObject_h
#define vm_ScopeObject_h

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"
#include "vm/ArgumentsObject.h"
#include "vm/ProxyObject.h"
#include "vm/WeakMapObject.h"

namespace js {

namespace frontend { struct Definition; }

class StaticWithObject;
class StaticEvalObject;























template <AllowGC allowGC>
class StaticScopeIter
{
    typename MaybeRooted<JSObject*, allowGC>::RootType obj;
    bool onNamedLambda;

  public:
    StaticScopeIter(ExclusiveContext* cx, JSObject* obj)
      : obj(cx, obj), onNamedLambda(false)
    {
        static_assert(allowGC == CanGC,
                      "the context-accepting constructor should only be used "
                      "in CanGC code");
        MOZ_ASSERT_IF(obj,
                      obj->is<StaticBlockObject>() ||
                      obj->is<StaticWithObject>() ||
                      obj->is<StaticEvalObject>() ||
                      obj->is<JSFunction>());
    }

    StaticScopeIter(ExclusiveContext* cx, const StaticScopeIter<CanGC>& ssi)
      : obj(cx, ssi.obj), onNamedLambda(ssi.onNamedLambda)
    {
        JS_STATIC_ASSERT(allowGC == CanGC);
    }

    explicit StaticScopeIter(JSObject* obj)
      : obj((ExclusiveContext*) nullptr, obj), onNamedLambda(false)
    {
        static_assert(allowGC == NoGC,
                      "the constructor not taking a context should only be "
                      "used in NoGC code");
        MOZ_ASSERT_IF(obj,
                      obj->is<StaticBlockObject>() ||
                      obj->is<StaticWithObject>() ||
                      obj->is<StaticEvalObject>() ||
                      obj->is<JSFunction>());
    }

    explicit StaticScopeIter(const StaticScopeIter<NoGC>& ssi)
      : obj((ExclusiveContext*) nullptr, ssi.obj), onNamedLambda(ssi.onNamedLambda)
    {
        static_assert(allowGC == NoGC,
                      "the constructor not taking a context should only be "
                      "used in NoGC code");
    }

    bool done() const;
    void operator++(int);

    
    bool hasDynamicScopeObject() const;
    Shape* scopeShape() const;

    enum Type { Function, Block, With, NamedLambda, Eval };
    Type type() const;

    StaticBlockObject& block() const;
    StaticWithObject& staticWith() const;
    StaticEvalObject& eval() const;
    JSScript* funScript() const;
    JSFunction& fun() const;
};









class ScopeCoordinate
{
    uint32_t hops_;
    uint32_t slot_;

    




    static_assert(SCOPECOORD_HOPS_BITS <= 32, "We have enough bits below");
    static_assert(SCOPECOORD_SLOT_BITS <= 32, "We have enough bits below");

  public:
    explicit inline ScopeCoordinate(jsbytecode* pc)
      : hops_(GET_SCOPECOORD_HOPS(pc)), slot_(GET_SCOPECOORD_SLOT(pc + SCOPECOORD_HOPS_LEN))
    {
        MOZ_ASSERT(JOF_OPTYPE(JSOp(*pc)) == JOF_SCOPECOORD);
    }

    inline ScopeCoordinate() {}

    void setHops(uint32_t hops) { MOZ_ASSERT(hops < SCOPECOORD_HOPS_LIMIT); hops_ = hops; }
    void setSlot(uint32_t slot) { MOZ_ASSERT(slot < SCOPECOORD_SLOT_LIMIT); slot_ = slot; }

    uint32_t hops() const { MOZ_ASSERT(hops_ < SCOPECOORD_HOPS_LIMIT); return hops_; }
    uint32_t slot() const { MOZ_ASSERT(slot_ < SCOPECOORD_SLOT_LIMIT); return slot_; }

    bool operator==(const ScopeCoordinate& rhs) const {
        return hops() == rhs.hops() && slot() == rhs.slot();
    }
};





extern Shape*
ScopeCoordinateToStaticScopeShape(JSScript* script, jsbytecode* pc);


extern PropertyName*
ScopeCoordinateName(ScopeCoordinateNameCache& cache, JSScript* script, jsbytecode* pc);


extern JSScript*
ScopeCoordinateFunctionScript(JSScript* script, jsbytecode* pc);













































class ScopeObject : public NativeObject
{
  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject& enclosingScope() const {
        return getFixedSlot(SCOPE_CHAIN_SLOT).toObject();
    }

    void setEnclosingScope(HandleObject obj);

    





    inline const Value& aliasedVar(ScopeCoordinate sc);

    inline void setAliasedVar(JSContext* cx, ScopeCoordinate sc, PropertyName* name, const Value& v);

    
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

    static CallObject*
    create(JSContext* cx, HandleScript script, HandleObject enclosing, HandleFunction callee);

    inline void initRemainingSlotsToUninitializedLexicals(uint32_t begin);
    inline void initAliasedLexicalsToThrowOnTouch(JSScript* script);

  public:
    static const Class class_;

    

    



    static CallObject*
    create(JSContext* cx, HandleShape shape, HandleObjectGroup group, uint32_t lexicalBegin);

    



    static CallObject*
    createSingleton(JSContext* cx, HandleShape shape, uint32_t lexicalBegin);

    static CallObject*
    createTemplateObject(JSContext* cx, HandleScript script, gc::InitialHeap heap);

    static const uint32_t RESERVED_SLOTS = 2;

    static CallObject* createForFunction(JSContext* cx, HandleObject enclosing, HandleFunction callee);

    static CallObject* createForFunction(JSContext* cx, AbstractFramePtr frame);
    static CallObject* createForStrictEval(JSContext* cx, AbstractFramePtr frame);
    static CallObject* createHollowForDebug(JSContext* cx, HandleFunction callee);

    
    bool isForEval() const {
        MOZ_ASSERT(getFixedSlot(CALLEE_SLOT).isObjectOrNull());
        MOZ_ASSERT_IF(getFixedSlot(CALLEE_SLOT).isObject(),
                      getFixedSlot(CALLEE_SLOT).toObject().is<JSFunction>());
        return getFixedSlot(CALLEE_SLOT).isNull();
    }

    



    JSFunction& callee() const {
        return getFixedSlot(CALLEE_SLOT).toObject().as<JSFunction>();
    }

    
    const Value& aliasedVar(AliasedFormalIter fi) {
        return getSlot(fi.scopeSlot());
    }
    inline void setAliasedVar(JSContext* cx, AliasedFormalIter fi, PropertyName* name,
                              const Value& v);

    







    const Value& aliasedVarFromArguments(const Value& argsValue) {
        return getSlot(ArgumentsObject::SlotFromMagicScopeSlotValue(argsValue));
    }
    inline void setAliasedVarFromArguments(JSContext* cx, const Value& argsValue, jsid id,
                                           const Value& v);

    
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
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT2_BACKGROUND;

    static const Class class_;

    static DeclEnvObject*
    createTemplateObject(JSContext* cx, HandleFunction fun, gc::InitialHeap heap);

    static DeclEnvObject* create(JSContext* cx, HandleObject enclosing, HandleFunction callee);

    static inline size_t lambdaSlot() {
        return LAMBDA_SLOT;
    }
};



class StaticEvalObject : public ScopeObject
{
    static const uint32_t STRICT_SLOT = 1;

  public:
    static const unsigned RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT2_BACKGROUND;

    static const Class class_;

    static StaticEvalObject* create(JSContext* cx, HandleObject enclosing);

    JSObject* enclosingScopeForStaticScopeIter() {
        return getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    void setStrict() {
        setReservedSlot(STRICT_SLOT, BooleanValue(true));
    }

    bool isStrict() const {
        return getReservedSlot(STRICT_SLOT).isTrue();
    }

    
    
    bool isDirect() const {
        return getReservedSlot(SCOPE_CHAIN_SLOT).isObject();
    }
};

class NestedScopeObject : public ScopeObject
{
  public:
    



    inline NestedScopeObject* enclosingNestedScope() const;

    
    inline bool isStatic() { return !getProto(); }

    
    inline NestedScopeObject* staticScope() {
        MOZ_ASSERT(!isStatic());
        return &getProto()->as<NestedScopeObject>();
    }

    
    JSObject* enclosingScopeForStaticScopeIter() {
        return getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    void initEnclosingNestedScope(JSObject* obj) {
        MOZ_ASSERT(getReservedSlot(SCOPE_CHAIN_SLOT).isUndefined());
        setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(obj));
    }

    






    void initEnclosingNestedScopeFromParser(NestedScopeObject* prev) {
        setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(prev));
    }

    void resetEnclosingNestedScopeFromParser() {
        setReservedSlot(SCOPE_CHAIN_SLOT, UndefinedValue());
    }
};


class StaticWithObject : public NestedScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 1;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT2_BACKGROUND;

    static const Class class_;

    static StaticWithObject* create(ExclusiveContext* cx);
};


class DynamicWithObject : public NestedScopeObject
{
    static const unsigned OBJECT_SLOT = 1;
    static const unsigned THIS_SLOT = 2;
    static const unsigned KIND_SLOT = 3;

  public:
    static const unsigned RESERVED_SLOTS = 4;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT4_BACKGROUND;

    static const Class class_;

    enum WithKind {
        SyntacticWith,
        NonSyntacticWith
    };

    static DynamicWithObject*
    create(JSContext* cx, HandleObject object, HandleObject enclosing, HandleObject staticWith,
           WithKind kind = SyntacticWith);

    StaticWithObject& staticWith() const {
        return getProto()->as<StaticWithObject>();
    }

    
    JSObject& object() const {
        return getReservedSlot(OBJECT_SLOT).toObject();
    }

    
    JSObject& withThis() const {
        return getReservedSlot(THIS_SLOT).toObject();
    }

    





    bool isSyntactic() const {
        return getReservedSlot(KIND_SLOT).toInt32() == SyntacticWith;
    }

    static inline size_t objectSlot() {
        return OBJECT_SLOT;
    }

    static inline size_t thisSlot() {
        return THIS_SLOT;
    }
};

class BlockObject : public NestedScopeObject
{
  protected:
    static const unsigned DEPTH_SLOT = 1;

  public:
    static const unsigned RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT4_BACKGROUND;

    static const Class class_;

    
    uint32_t stackDepth() const {
        return getReservedSlot(DEPTH_SLOT).toPrivateUint32();
    }

    
    uint32_t numVariables() const {
        
        return propertyCount();
    }

  protected:
    
    const Value& slotValue(unsigned i) {
        return getSlotRef(RESERVED_SLOTS + i);
    }

    void setSlotValue(unsigned i, const Value& v) {
        setSlot(RESERVED_SLOTS + i, v);
    }
};

class StaticBlockObject : public BlockObject
{
    static const unsigned LOCAL_OFFSET_SLOT = 1;

  public:
    static StaticBlockObject* create(ExclusiveContext* cx);

    
    JSObject* enclosingStaticScope() const {
        return getFixedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    



    uint32_t shapeToIndex(const Shape& shape) {
        uint32_t slot = shape.slot();
        MOZ_ASSERT(slot - RESERVED_SLOTS < numVariables());
        return slot - RESERVED_SLOTS;
    }

    



    inline StaticBlockObject* enclosingBlock() const;

    uint32_t localOffset() {
        return getReservedSlot(LOCAL_OFFSET_SLOT).toPrivateUint32();
    }

    
    
    uint32_t blockIndexToLocalIndex(uint32_t index) {
        MOZ_ASSERT(index < numVariables());
        return getReservedSlot(LOCAL_OFFSET_SLOT).toPrivateUint32() + index;
    }

    
    
    
    uint32_t localIndexToSlot(uint32_t local) {
        MOZ_ASSERT(local >= localOffset());
        local -= localOffset();
        MOZ_ASSERT(local < numVariables());
        return RESERVED_SLOTS + local;
    }

    



    bool isAliased(unsigned i) {
        return slotValue(i).isTrue();
    }

    



    bool needsClone() {
        return !getFixedSlot(RESERVED_SLOTS).isFalse();
    }

    

    
    void setAliased(unsigned i, bool aliased) {
        MOZ_ASSERT_IF(i > 0, slotValue(i-1).isBoolean());
        setSlotValue(i, BooleanValue(aliased));
        if (aliased && !needsClone()) {
            setSlotValue(0, MagicValue(JS_BLOCK_NEEDS_CLONE));
            MOZ_ASSERT(needsClone());
        }
    }

    void setLocalOffset(uint32_t offset) {
        MOZ_ASSERT(getReservedSlot(LOCAL_OFFSET_SLOT).isUndefined());
        initReservedSlot(LOCAL_OFFSET_SLOT, PrivateUint32Value(offset));
    }

    



    void setDefinitionParseNode(unsigned i, frontend::Definition* def) {
        MOZ_ASSERT(slotValue(i).isUndefined());
        setSlotValue(i, PrivateValue(def));
    }

    frontend::Definition* definitionParseNode(unsigned i) {
        Value v = slotValue(i);
        return reinterpret_cast<frontend::Definition*>(v.toPrivate());
    }

    





    static const unsigned LOCAL_INDEX_LIMIT = JS_BIT(16);

    static Shape* addVar(ExclusiveContext* cx, Handle<StaticBlockObject*> block, HandleId id,
                         bool constant, unsigned index, bool* redeclared);
};

class ClonedBlockObject : public BlockObject
{
    static ClonedBlockObject* create(JSContext* cx, Handle<StaticBlockObject*> block,
                                     HandleObject enclosing);

  public:
    static ClonedBlockObject* create(JSContext* cx, Handle<StaticBlockObject*> block,
                                     AbstractFramePtr frame);

    static ClonedBlockObject* createHollowForDebug(JSContext* cx,
                                                   Handle<StaticBlockObject*> block);

    
    StaticBlockObject& staticBlock() const {
        return getProto()->as<StaticBlockObject>();
    }

    
    const Value& var(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        MOZ_ASSERT_IF(checkAliasing, staticBlock().isAliased(i));
        return slotValue(i);
    }

    void setVar(unsigned i, const Value& v, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        MOZ_ASSERT_IF(checkAliasing, staticBlock().isAliased(i));
        setSlotValue(i, v);
    }

    
    void copyUnaliasedValues(AbstractFramePtr frame);

    



    static ClonedBlockObject* clone(JSContext* cx, Handle<ClonedBlockObject*> block);
};

















class UninitializedLexicalObject : public ScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 1;
    static const gc::AllocKind FINALIZE_KIND = gc::AllocKind::OBJECT2_BACKGROUND;

    static const Class class_;

    static UninitializedLexicalObject* create(JSContext* cx, HandleObject enclosing);
};

template<XDRMode mode>
bool
XDRStaticBlockObject(XDRState<mode>* xdr, HandleObject enclosingScope,
                     MutableHandle<StaticBlockObject*> objp);

template<XDRMode mode>
bool
XDRStaticWithObject(XDRState<mode>* xdr, HandleObject enclosingScope,
                    MutableHandle<StaticWithObject*> objp);

extern JSObject*
CloneNestedScopeObject(JSContext* cx, HandleObject enclosingScope, Handle<NestedScopeObject*> src);









class ScopeIter
{
    StaticScopeIter<CanGC> ssi_;
    RootedObject scope_;
    AbstractFramePtr frame_;

    void incrementStaticScopeIter();
    void settle();

    
    ScopeIter(const ScopeIter& si) = delete;

  public:
    
    ScopeIter(JSContext* cx, const ScopeIter& si
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    
    ScopeIter(JSContext* cx, JSObject* scope, JSObject* staticScope
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    
    ScopeIter(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    inline bool done() const;
    ScopeIter& operator++();

    
    inline JSObject& enclosingScope() const;

    
    enum Type { Call, Block, With, Eval };
    Type type() const;

    inline bool hasScopeObject() const;
    inline bool canHaveScopeObject() const;
    ScopeObject& scope() const;

    JSObject* maybeStaticScope() const;
    StaticBlockObject& staticBlock() const { return ssi_.block(); }
    StaticWithObject& staticWith() const { return ssi_.staticWith(); }
    StaticEvalObject& staticEval() const { return ssi_.eval(); }
    JSFunction& fun() const { return ssi_.fun(); }

    bool withinInitialFrame() const { return !!frame_; }
    AbstractFramePtr initialFrame() const { MOZ_ASSERT(withinInitialFrame()); return frame_; }
    AbstractFramePtr maybeInitialFrame() const { return frame_; }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};













class MissingScopeKey
{
    friend class LiveScopeVal;

    AbstractFramePtr frame_;
    JSObject* staticScope_;

  public:
    explicit MissingScopeKey(const ScopeIter& si)
      : frame_(si.maybeInitialFrame()),
        staticScope_(si.maybeStaticScope())
    { }

    AbstractFramePtr frame() const { return frame_; }
    JSObject* staticScope() const { return staticScope_; }

    void updateStaticScope(JSObject* obj) { staticScope_ = obj; }
    void updateFrame(AbstractFramePtr frame) { frame_ = frame; }

    
    typedef MissingScopeKey Lookup;
    static HashNumber hash(MissingScopeKey sk);
    static bool match(MissingScopeKey sk1, MissingScopeKey sk2);
    bool operator!=(const MissingScopeKey& other) const {
        return frame_ != other.frame_ || staticScope_ != other.staticScope_;
    }
    static void rekey(MissingScopeKey& k, const MissingScopeKey& newKey) {
        k = newKey;
    }
};


class LiveScopeVal
{
    friend class DebugScopes;
    friend class MissingScopeKey;

    AbstractFramePtr frame_;
    RelocatablePtrObject staticScope_;

    void sweep();
    static void staticAsserts();

  public:
    explicit LiveScopeVal(const ScopeIter& si)
      : frame_(si.initialFrame()),
        staticScope_(si.maybeStaticScope())
    { }

    AbstractFramePtr frame() const { return frame_; }
    JSObject* staticScope() const { return staticScope_; }

    void updateFrame(AbstractFramePtr frame) { frame_ = frame; }
};




























extern JSObject*
GetDebugScopeForFunction(JSContext* cx, HandleFunction fun);

extern JSObject*
GetDebugScopeForFrame(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc);


class DebugScopeObject : public ProxyObject
{
    



    static const unsigned ENCLOSING_EXTRA = 0;

    



    static const unsigned SNAPSHOT_EXTRA = 1;

  public:
    static DebugScopeObject* create(JSContext* cx, ScopeObject& scope, HandleObject enclosing);

    ScopeObject& scope() const;
    JSObject& enclosingScope() const;

    
    ArrayObject* maybeSnapshot() const;
    void initSnapshot(ArrayObject& snapshot);

    
    bool isForDeclarative() const;

    
    
    bool getMaybeSentinelValue(JSContext* cx, HandleId id, MutableHandleValue vp);

    
    
    bool isOptimizedOut() const;
};


class DebugScopes
{
    
    ObjectWeakMap proxiedScopes;

    



    typedef HashMap<MissingScopeKey,
                    ReadBarrieredDebugScopeObject,
                    MissingScopeKey,
                    RuntimeAllocPolicy> MissingScopeMap;
    MissingScopeMap missingScopes;

    






    typedef HashMap<ScopeObject*,
                    LiveScopeVal,
                    DefaultHasher<ScopeObject*>,
                    RuntimeAllocPolicy> LiveScopeMap;
    LiveScopeMap liveScopes;
    static MOZ_ALWAYS_INLINE void liveScopesPostWriteBarrier(JSRuntime* rt, LiveScopeMap* map,
                                                             ScopeObject* key);

  public:
    explicit DebugScopes(JSContext* c);
    ~DebugScopes();

  private:
    bool init();

    static DebugScopes* ensureCompartmentData(JSContext* cx);

  public:
    void mark(JSTracer* trc);
    void sweep(JSRuntime* rt);
#ifdef JS_GC_ZEAL
    void checkHashTablesAfterMovingGC(JSRuntime* rt);
#endif

    static DebugScopeObject* hasDebugScope(JSContext* cx, ScopeObject& scope);
    static bool addDebugScope(JSContext* cx, ScopeObject& scope, DebugScopeObject& debugScope);

    static DebugScopeObject* hasDebugScope(JSContext* cx, const ScopeIter& si);
    static bool addDebugScope(JSContext* cx, const ScopeIter& si, DebugScopeObject& debugScope);

    static bool updateLiveScopes(JSContext* cx);
    static LiveScopeVal* hasLiveScope(ScopeObject& scope);
    static void unsetPrevUpToDateUntil(JSContext* cx, AbstractFramePtr frame);

    
    
    
    static void forwardLiveFrame(JSContext* cx, AbstractFramePtr from, AbstractFramePtr to);

    
    
    static void onPopCall(AbstractFramePtr frame, JSContext* cx);
    static void onPopBlock(JSContext* cx, const ScopeIter& si);
    static void onPopBlock(JSContext* cx, AbstractFramePtr frame, jsbytecode* pc);
    static void onPopWith(AbstractFramePtr frame);
    static void onPopStrictEvalScope(AbstractFramePtr frame);
    static void onCompartmentUnsetIsDebuggee(JSCompartment* c);
};

extern bool
IsDebugScopeSlow(ProxyObject* proxy);

}  

template<>
inline bool
JSObject::is<js::NestedScopeObject>() const
{
    return is<js::BlockObject>() ||
           is<js::StaticWithObject>() ||
           is<js::DynamicWithObject>();
}

template<>
inline bool
JSObject::is<js::ScopeObject>() const
{
    return is<js::CallObject>() ||
           is<js::DeclEnvObject>() ||
           is<js::NestedScopeObject>() ||
           is<js::UninitializedLexicalObject>();
}

template<>
inline bool
JSObject::is<js::DebugScopeObject>() const
{
    
    return hasClass(&js::ProxyObject::class_) &&
           IsDebugScopeSlow(&const_cast<JSObject*>(this)->as<js::ProxyObject>());
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

namespace js {

inline bool
IsSyntacticScope(JSObject* scope)
{
    return scope->is<ScopeObject>() &&
           (!scope->is<DynamicWithObject>() ||
            scope->as<DynamicWithObject>().isSyntactic());
}

inline const Value&
ScopeObject::aliasedVar(ScopeCoordinate sc)
{
    MOZ_ASSERT(is<CallObject>() || is<ClonedBlockObject>());
    return getSlot(sc.slot());
}

inline NestedScopeObject*
NestedScopeObject::enclosingNestedScope() const
{
    JSObject* obj = getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    return obj && obj->is<NestedScopeObject>() ? &obj->as<NestedScopeObject>() : nullptr;
}

inline bool
ScopeIter::done() const
{
    return ssi_.done();
}

inline bool
ScopeIter::hasScopeObject() const
{
    return ssi_.hasDynamicScopeObject();
}

inline bool
ScopeIter::canHaveScopeObject() const
{
    
    return !ssi_.done() && (type() != Eval || staticEval().isStrict());
}

inline JSObject&
ScopeIter::enclosingScope() const
{
    
    
    
    
    MOZ_ASSERT(done());
    MOZ_ASSERT(!IsSyntacticScope(scope_));
    return *scope_;
}

extern bool
CreateScopeObjectsForScopeChain(JSContext* cx, AutoObjectVector& scopeChain,
                                HandleObject dynamicTerminatingScope,
                                MutableHandleObject dynamicScopeObj,
                                MutableHandleObject staticScopeObj);

#ifdef DEBUG
bool
AnalyzeEntrainedVariables(JSContext* cx, HandleScript script);
#endif

} 

#endif 
