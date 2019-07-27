





#ifndef vm_ScopeObject_h
#define vm_ScopeObject_h

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"
#include "vm/ProxyObject.h"

namespace js {

namespace frontend { struct Definition; }

class StaticWithObject;





































template <AllowGC allowGC>
class StaticScopeIter
{
    typename MaybeRooted<JSObject*, allowGC>::RootType obj;
    bool onNamedLambda;

  public:
    StaticScopeIter(ExclusiveContext *cx, JSObject *obj)
      : obj(cx, obj), onNamedLambda(false)
    {
        JS_STATIC_ASSERT(allowGC == CanGC);
        JS_ASSERT_IF(obj, obj->is<StaticBlockObject>() || obj->is<StaticWithObject>() ||
                     obj->is<JSFunction>());
    }

    explicit StaticScopeIter(JSObject *obj)
      : obj((ExclusiveContext *) nullptr, obj), onNamedLambda(false)
    {
        JS_STATIC_ASSERT(allowGC == NoGC);
        JS_ASSERT_IF(obj, obj->is<StaticBlockObject>() || obj->is<StaticWithObject>() ||
                     obj->is<JSFunction>());
    }

    bool done() const;
    void operator++(int);

    
    bool hasDynamicScopeObject() const;
    Shape *scopeShape() const;

    enum Type { WITH, BLOCK, FUNCTION, NAMED_LAMBDA };
    Type type() const;

    StaticBlockObject &block() const;
    StaticWithObject &staticWith() const;
    JSScript *funScript() const;
    JSFunction &fun() const;
};









class ScopeCoordinate
{
    uint32_t hops_;
    uint32_t slot_;

    




    static_assert(SCOPECOORD_HOPS_BITS <= 32, "We have enough bits below");
    static_assert(SCOPECOORD_SLOT_BITS <= 32, "We have enough bits below");

  public:
    explicit inline ScopeCoordinate(jsbytecode *pc)
      : hops_(GET_SCOPECOORD_HOPS(pc)), slot_(GET_SCOPECOORD_SLOT(pc + SCOPECOORD_HOPS_LEN))
    {
        JS_ASSERT(JOF_OPTYPE(*pc) == JOF_SCOPECOORD);
    }

    inline ScopeCoordinate() {}

    void setHops(uint32_t hops) { JS_ASSERT(hops < SCOPECOORD_HOPS_LIMIT); hops_ = hops; }
    void setSlot(uint32_t slot) { JS_ASSERT(slot < SCOPECOORD_SLOT_LIMIT); slot_ = slot; }

    uint32_t hops() const { JS_ASSERT(hops_ < SCOPECOORD_HOPS_LIMIT); return hops_; }
    uint32_t slot() const { JS_ASSERT(slot_ < SCOPECOORD_SLOT_LIMIT); return slot_; }

    bool operator==(const ScopeCoordinate &rhs) const {
        return hops() == rhs.hops() && slot() == rhs.slot();
    }
};





extern Shape *
ScopeCoordinateToStaticScopeShape(JSScript *script, jsbytecode *pc);


extern PropertyName *
ScopeCoordinateName(ScopeCoordinateNameCache &cache, JSScript *script, jsbytecode *pc);


extern JSScript *
ScopeCoordinateFunctionScript(JSScript *script, jsbytecode *pc);











































class ScopeObject : public JSObject
{
  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject &enclosingScope() const {
        return getFixedSlot(SCOPE_CHAIN_SLOT).toObject();
    }

    void setEnclosingScope(HandleObject obj);

    





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

    inline void setAliasedLexicalsToThrowOnTouch(JSScript *script);

  public:
    static const Class class_;

    

    



    static CallObject *
    create(JSContext *cx, HandleShape shape, HandleTypeObject type);

    



    static CallObject *
    createSingleton(JSContext *cx, HandleShape shape);

    static CallObject *
    createTemplateObject(JSContext *cx, HandleScript script, gc::InitialHeap heap);

    static const uint32_t RESERVED_SLOTS = 2;

    static CallObject *createForFunction(JSContext *cx, HandleObject enclosing, HandleFunction callee);

    static CallObject *createForFunction(JSContext *cx, AbstractFramePtr frame);
    static CallObject *createForStrictEval(JSContext *cx, AbstractFramePtr frame);

    
    bool isForEval() const {
        JS_ASSERT(getFixedSlot(CALLEE_SLOT).isObjectOrNull());
        JS_ASSERT_IF(getFixedSlot(CALLEE_SLOT).isObject(),
                     getFixedSlot(CALLEE_SLOT).toObject().is<JSFunction>());
        return getFixedSlot(CALLEE_SLOT).isNull();
    }

    



    JSFunction &callee() const {
        return getFixedSlot(CALLEE_SLOT).toObject().as<JSFunction>();
    }

    
    const Value &aliasedVar(AliasedFormalIter fi) {
        return getSlot(fi.scopeSlot());
    }
    inline void setAliasedVar(JSContext *cx, AliasedFormalIter fi, PropertyName *name,
                              const Value &v);

    







    const Value &aliasedVarFromArguments(const Value &argsValue) {
        return getSlot(argsValue.magicUint32());
    }
    inline void setAliasedVarFromArguments(JSContext *cx, const Value &argsValue, jsid id,
                                           const Value &v);

    
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
  public:
    



    inline NestedScopeObject *enclosingNestedScope() const;

    
    inline bool isStatic() { return !getProto(); }

    
    inline NestedScopeObject* staticScope() {
        JS_ASSERT(!isStatic());
        return &getProto()->as<NestedScopeObject>();
    }

    
    JSObject *enclosingScopeForStaticScopeIter() {
        return getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    void initEnclosingNestedScope(JSObject *obj) {
        JS_ASSERT(getReservedSlot(SCOPE_CHAIN_SLOT).isUndefined());
        setReservedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(obj));
    }

    






    void initEnclosingNestedScopeFromParser(NestedScopeObject *prev) {
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
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT2_BACKGROUND;

    static const Class class_;

    static StaticWithObject *create(ExclusiveContext *cx);
};


class DynamicWithObject : public NestedScopeObject
{
    static const unsigned OBJECT_SLOT = 1;
    static const unsigned THIS_SLOT = 2;

  public:
    static const unsigned RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    static const Class class_;

    static DynamicWithObject *
    create(JSContext *cx, HandleObject object, HandleObject enclosing, HandleObject staticWith);

    StaticWithObject& staticWith() const {
        return getProto()->as<StaticWithObject>();
    }

    
    JSObject &object() const {
        return getReservedSlot(OBJECT_SLOT).toObject();
    }

    
    JSObject &withThis() const {
        return getReservedSlot(THIS_SLOT).toObject();
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
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;

    static const Class class_;

    
    uint32_t stackDepth() const {
        return getReservedSlot(DEPTH_SLOT).toPrivateUint32();
    }

    
    uint32_t numVariables() const {
        
        return propertyCount();
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
    static const unsigned LOCAL_OFFSET_SLOT = 1;

  public:
    static StaticBlockObject *create(ExclusiveContext *cx);

    
    JSObject *enclosingStaticScope() const {
        return getFixedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    }

    



    uint32_t shapeToIndex(const Shape &shape) {
        uint32_t slot = shape.slot();
        JS_ASSERT(slot - RESERVED_SLOTS < numVariables());
        return slot - RESERVED_SLOTS;
    }

    



    inline StaticBlockObject *enclosingBlock() const;

    uint32_t localOffset() {
        return getReservedSlot(LOCAL_OFFSET_SLOT).toPrivateUint32();
    }

    
    
    uint32_t blockIndexToLocalIndex(uint32_t index) {
        JS_ASSERT(index < numVariables());
        return getReservedSlot(LOCAL_OFFSET_SLOT).toPrivateUint32() + index;
    }

    
    
    
    uint32_t localIndexToSlot(uint32_t local) {
        JS_ASSERT(local >= localOffset());
        local -= localOffset();
        JS_ASSERT(local < numVariables());
        return RESERVED_SLOTS + local;
    }

    



    bool isAliased(unsigned i) {
        return slotValue(i).isTrue();
    }

    



    bool needsClone() {
        return !getFixedSlot(RESERVED_SLOTS).isFalse();
    }

    

    
    void setAliased(unsigned i, bool aliased) {
        JS_ASSERT_IF(i > 0, slotValue(i-1).isBoolean());
        setSlotValue(i, BooleanValue(aliased));
        if (aliased && !needsClone()) {
            setSlotValue(0, MagicValue(JS_BLOCK_NEEDS_CLONE));
            JS_ASSERT(needsClone());
        }
    }

    void setLocalOffset(uint32_t offset) {
        JS_ASSERT(getReservedSlot(LOCAL_OFFSET_SLOT).isUndefined());
        initReservedSlot(LOCAL_OFFSET_SLOT, PrivateUint32Value(offset));
    }

    



    void setDefinitionParseNode(unsigned i, frontend::Definition *def) {
        JS_ASSERT(slotValue(i).isUndefined());
        setSlotValue(i, PrivateValue(def));
    }

    frontend::Definition *definitionParseNode(unsigned i) {
        Value v = slotValue(i);
        return reinterpret_cast<frontend::Definition *>(v.toPrivate());
    }

    





    static const unsigned LOCAL_INDEX_LIMIT = JS_BIT(16);

    static Shape *addVar(ExclusiveContext *cx, Handle<StaticBlockObject*> block, HandleId id,
                         unsigned index, bool *redeclared);
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

















class UninitializedLexicalObject : public ScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 1;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT2_BACKGROUND;

    static const Class class_;

    static UninitializedLexicalObject *create(JSContext *cx, HandleObject enclosing);
};

template<XDRMode mode>
bool
XDRStaticBlockObject(XDRState<mode> *xdr, HandleObject enclosingScope,
                     StaticBlockObject **objp);

template<XDRMode mode>
bool
XDRStaticWithObject(XDRState<mode> *xdr, HandleObject enclosingScope,
                    StaticWithObject **objp);

extern JSObject *
CloneNestedScopeObject(JSContext *cx, HandleObject enclosingScope, Handle<NestedScopeObject*> src);



class ScopeIterKey;
class ScopeIterVal;














class ScopeIter
{
    friend class ScopeIterKey;
    friend class ScopeIterVal;

  public:
    enum Type { Call, Block, With, StrictEvalScope };

  private:
    JSContext *cx;
    AbstractFramePtr frame_;
    RootedObject cur_;
    Rooted<NestedScopeObject *> staticScope_;
    Type type_;
    bool hasScopeObject_;

    void settle();

    
    ScopeIter(const ScopeIter &si) MOZ_DELETE;

    ScopeIter(JSContext *cx) MOZ_DELETE;

  public:

    
    ScopeIter(const ScopeIter &si, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    
    ScopeIter(AbstractFramePtr frame, jsbytecode *pc, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    



    ScopeIter(JSObject &enclosingScope, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    ScopeIter(const ScopeIterVal &hashVal, JSContext *cx
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    bool done() const { return !frame_; }

    

    JSObject &enclosingScope() const { JS_ASSERT(done()); return *cur_; }

    

    ScopeIter &operator++();

    AbstractFramePtr frame() const { JS_ASSERT(!done()); return frame_; }
    Type type() const { JS_ASSERT(!done()); return type_; }
    bool hasScopeObject() const { JS_ASSERT(!done()); return hasScopeObject_; }
    ScopeObject &scope() const;
    NestedScopeObject* staticScope() const { return staticScope_; }

    StaticBlockObject &staticBlock() const {
        JS_ASSERT(type() == Block);
        return staticScope_->as<StaticBlockObject>();
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class ScopeIterKey
{
    friend class ScopeIterVal;

    AbstractFramePtr frame_;
    JSObject *cur_;
    NestedScopeObject *staticScope_;
    ScopeIter::Type type_;
    bool hasScopeObject_;

  public:
    explicit ScopeIterKey(const ScopeIter &si)
      : frame_(si.frame()), cur_(si.cur_), staticScope_(si.staticScope_), type_(si.type_),
        hasScopeObject_(si.hasScopeObject_) {}

    AbstractFramePtr frame() const { return frame_; }
    JSObject *cur() const { return cur_; }
    NestedScopeObject *staticScope() const { return staticScope_; }
    ScopeIter::Type type() const { return type_; }
    bool hasScopeObject() const { return hasScopeObject_; }
    JSObject *enclosingScope() const { return cur_; }
    JSObject *&enclosingScope() { return cur_; }

    void updateCur(JSObject *obj) { cur_ = obj; }
    void updateStaticScope(NestedScopeObject *obj) { staticScope_ = obj; }

    
    typedef ScopeIterKey Lookup;
    static HashNumber hash(ScopeIterKey si);
    static bool match(ScopeIterKey si1, ScopeIterKey si2);
    bool operator!=(const ScopeIterKey &other) const {
        return frame_ != other.frame_ ||
               cur_ != other.cur_ ||
               staticScope_ != other.staticScope_ ||
               type_ != other.type_;
    }
    static void rekey(ScopeIterKey &k, const ScopeIterKey& newKey) {
        k = newKey;
    }
};

class ScopeIterVal
{
    friend class ScopeIter;
    friend class DebugScopes;

    AbstractFramePtr frame_;
    RelocatablePtrObject cur_;
    RelocatablePtrNestedScopeObject staticScope_;
    ScopeIter::Type type_;
    bool hasScopeObject_;

    void sweep();

    static void staticAsserts();

  public:
    explicit ScopeIterVal(const ScopeIter &si)
      : frame_(si.frame()), cur_(si.cur_), staticScope_(si.staticScope_), type_(si.type_),
        hasScopeObject_(si.hasScopeObject_) {}

    AbstractFramePtr frame() const { return frame_; }
};




























extern JSObject *
GetDebugScopeForFunction(JSContext *cx, HandleFunction fun);

extern JSObject *
GetDebugScopeForFrame(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc);


class DebugScopeObject : public ProxyObject
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

    
    
    bool getMaybeSentinelValue(JSContext *cx, HandleId id, MutableHandleValue vp);
};


class DebugScopes
{
    
    typedef WeakMap<PreBarrieredObject, RelocatablePtrObject> ObjectWeakMap;
    ObjectWeakMap proxiedScopes;
    static MOZ_ALWAYS_INLINE void proxiedScopesPostWriteBarrier(JSRuntime *rt, ObjectWeakMap *map,
                                                               const PreBarrieredObject &key);

    



    typedef HashMap<ScopeIterKey,
                    ReadBarrieredDebugScopeObject,
                    ScopeIterKey,
                    RuntimeAllocPolicy> MissingScopeMap;
    MissingScopeMap missingScopes;
    class MissingScopesRef;
    static MOZ_ALWAYS_INLINE void missingScopesPostWriteBarrier(JSRuntime *rt, MissingScopeMap *map,
                                                               const ScopeIterKey &key);

    






    typedef HashMap<ScopeObject *,
                    ScopeIterVal,
                    DefaultHasher<ScopeObject *>,
                    RuntimeAllocPolicy> LiveScopeMap;
    LiveScopeMap liveScopes;
    static MOZ_ALWAYS_INLINE void liveScopesPostWriteBarrier(JSRuntime *rt, LiveScopeMap *map,
                                                            ScopeObject *key);

  public:
    explicit DebugScopes(JSContext *c);
    ~DebugScopes();

  private:
    bool init();

    static DebugScopes *ensureCompartmentData(JSContext *cx);

  public:
    void mark(JSTracer *trc);
    void sweep(JSRuntime *rt);
#if defined(JSGC_GENERATIONAL) && defined(JS_GC_ZEAL)
    void checkHashTablesAfterMovingGC(JSRuntime *rt);
#endif

    static DebugScopeObject *hasDebugScope(JSContext *cx, ScopeObject &scope);
    static bool addDebugScope(JSContext *cx, ScopeObject &scope, DebugScopeObject &debugScope);

    static DebugScopeObject *hasDebugScope(JSContext *cx, const ScopeIter &si);
    static bool addDebugScope(JSContext *cx, const ScopeIter &si, DebugScopeObject &debugScope);

    static bool updateLiveScopes(JSContext *cx);
    static ScopeIterVal *hasLiveScope(ScopeObject &scope);

    
    
    static void onPopCall(AbstractFramePtr frame, JSContext *cx);
    static void onPopBlock(JSContext *cx, const ScopeIter &si);
    static void onPopBlock(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc);
    static void onPopWith(AbstractFramePtr frame);
    static void onPopStrictEvalScope(AbstractFramePtr frame);
    static void onCompartmentLeaveDebugMode(JSCompartment *c);
};

}  

template<>
inline bool
JSObject::is<js::NestedScopeObject>() const
{
    return is<js::BlockObject>() || is<js::StaticWithObject>() || is<js::DynamicWithObject>();
}

template<>
inline bool
JSObject::is<js::ScopeObject>() const
{
    return is<js::CallObject>() || is<js::DeclEnvObject>() || is<js::NestedScopeObject>() ||
           is<js::UninitializedLexicalObject>();
}

template<>
inline bool
JSObject::is<js::DebugScopeObject>() const
{
    extern bool js_IsDebugScopeSlow(js::ProxyObject *proxy);

    
    return hasClass(&js::ProxyObject::class_) &&
           js_IsDebugScopeSlow(&const_cast<JSObject*>(this)->as<js::ProxyObject>());
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
    return getSlot(sc.slot());
}

inline NestedScopeObject *
NestedScopeObject::enclosingNestedScope() const
{
    JSObject *obj = getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    return obj && obj->is<NestedScopeObject>() ? &obj->as<NestedScopeObject>() : nullptr;
}

#ifdef DEBUG
bool
AnalyzeEntrainedVariables(JSContext *cx, HandleScript script);
#endif

} 

#endif
