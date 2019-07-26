






#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

#include "gc/Barrier.h"

namespace js {












struct ScopeCoordinate
{
    uint16_t hops;
    uint16_t slot;

    inline ScopeCoordinate(jsbytecode *pc);
    inline ScopeCoordinate() {}
};


extern StaticBlockObject *
ScopeCoordinateBlockChain(JSScript *script, jsbytecode *pc);


extern PropertyName *
ScopeCoordinateName(JSRuntime *rt, JSScript *script, jsbytecode *pc);









































class ScopeObject : public JSObject
{
  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    
    static const uint32_t CALL_BLOCK_RESERVED_SLOTS = 2;

    




    inline JSObject &enclosingScope() const;
    inline void setEnclosingScope(HandleObject obj);

    





    inline const Value &aliasedVar(ScopeCoordinate sc);
    inline void setAliasedVar(ScopeCoordinate sc, const Value &v);

    
    static inline size_t offsetOfEnclosingScope();
};

class CallObject : public ScopeObject
{
    static const uint32_t CALLEE_SLOT = 1;

    static CallObject *
    create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleFunction callee);

  public:
    static const uint32_t RESERVED_SLOTS = CALL_BLOCK_RESERVED_SLOTS;

    static CallObject *createForFunction(JSContext *cx, StackFrame *fp);
    static CallObject *createForStrictEval(JSContext *cx, StackFrame *fp);

    
    inline bool isForEval() const;

    



    inline JSObject *getCallee() const;
    inline JSFunction *getCalleeFunction() const;
    inline void setCallee(JSObject *callee);

    
    inline const Value &arg(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;
    inline void setArg(unsigned i, const Value &v, MaybeCheckAliasing = CHECK_ALIASING);

    
    inline const Value &var(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;
    inline void setVar(unsigned i, const Value &v, MaybeCheckAliasing = CHECK_ALIASING);

    




    inline HeapSlotArray argArray();
    inline HeapSlotArray varArray();

    static JSBool setArgOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp);
    static JSBool setVarOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp);

    
    void copyUnaliasedValues(StackFrame *fp);
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
#ifdef JS_THREADSAFE
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;
#else
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;
#endif

    static WithObject *
    create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth);

    
    JSObject &withThis() const;

    
    JSObject &object() const;
};

class BlockObject : public NestedScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = CALL_BLOCK_RESERVED_SLOTS;
#ifdef JS_THREADSAFE
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4_BACKGROUND;
#else
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;
#endif

    
    inline uint32_t slotCount() const;

    




    unsigned slotToFrameLocal(JSScript *script, unsigned i);

  protected:
    
    inline const Value &slotValue(unsigned i);
    inline void setSlotValue(unsigned i, const Value &v);
};

class StaticBlockObject : public BlockObject
{
  public:
    static StaticBlockObject *create(JSContext *cx);

    inline StaticBlockObject *enclosingBlock() const;
    inline void setEnclosingBlock(StaticBlockObject *blockObj);

    void setStackDepth(uint32_t depth);
    bool containsVarAtDepth(uint32_t depth);

    



    void setDefinitionParseNode(unsigned i, Definition *def);
    Definition *maybeDefinitionParseNode(unsigned i);

    



    void setAliased(unsigned i, bool aliased);
    bool isAliased(unsigned i);

    



    bool needsClone();

    const Shape *addVar(JSContext *cx, jsid id, int index, bool *redeclared);
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
XDRStaticBlockObject(XDRState<mode> *xdr, JSScript *script, StaticBlockObject **objp);

extern JSObject *
CloneStaticBlockObject(JSContext *cx, StaticBlockObject &srcBlock,
                       const AutoObjectVector &objects, JSScript *src);
















class ScopeIter
{
  public:
    enum Type { Call, Block, With, StrictEvalScope };

  private:
    StackFrame *fp_;
    JSObject *cur_;
    StaticBlockObject *block_;
    Type type_;
    bool hasScopeObject_;

    void settle();

  public:
    
    explicit ScopeIter();

    
    explicit ScopeIter(StackFrame *fp);

    



    explicit ScopeIter(JSObject &enclosingScope);

    



    ScopeIter(ScopeIter si, StackFrame *fp);

    
    ScopeIter(StackFrame *fp, ScopeObject &scope);

    bool done() const { return !fp_; }

    

    JSObject &enclosingScope() const { JS_ASSERT(done()); return *cur_; }

    

    ScopeIter enclosing() const;

    StackFrame *fp() const { JS_ASSERT(!done()); return fp_; }
    Type type() const { JS_ASSERT(!done()); return type_; }
    bool hasScopeObject() const { JS_ASSERT(!done()); return hasScopeObject_; }
    ScopeObject &scope() const;

    StaticBlockObject &staticBlock() const { JS_ASSERT(type() == Block); return *block_; }

    
    typedef ScopeIter Lookup;
    static HashNumber hash(ScopeIter si);
    static bool match(ScopeIter si1, ScopeIter si2);
};




























extern JSObject *
GetDebugScopeForFunction(JSContext *cx, JSFunction *fun);

extern JSObject *
GetDebugScopeForFrame(JSContext *cx, StackFrame *fp);


class DebugScopeObject : public JSObject
{
    static const unsigned ENCLOSING_EXTRA = 0;

  public:
    static DebugScopeObject *create(JSContext *cx, ScopeObject &scope, HandleObject enclosing);

    ScopeObject &scope() const;
    JSObject &enclosingScope() const;

    
    bool isForDeclarative() const;
};


class DebugScopes
{
    JSRuntime *rt;

    
    typedef WeakMap<HeapPtrObject, HeapPtrObject> ObjectWeakMap;
    ObjectWeakMap proxiedScopes;

    



    typedef HashMap<ScopeIter,
                    ReadBarriered<DebugScopeObject>,
                    ScopeIter,
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

    DebugScopeObject *hasDebugScope(JSContext *cx, ScopeIter si) const;
    bool addDebugScope(JSContext *cx, ScopeIter si, DebugScopeObject &debugScope);

    bool updateLiveScopes(JSContext *cx);
    StackFrame *hasLiveFrame(ScopeObject &scope);

    



    void onPopCall(StackFrame *fp);
    void onPopBlock(JSContext *cx, StackFrame *fp);
    void onPopWith(StackFrame *fp);
    void onPopStrictEvalScope(StackFrame *fp);
    void onGeneratorFrameChange(StackFrame *from, StackFrame *to);
    void onCompartmentLeaveDebugMode(JSCompartment *c);
};

}  
#endif
