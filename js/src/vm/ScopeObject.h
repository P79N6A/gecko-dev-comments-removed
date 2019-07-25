








































#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "jscntxt.h"
#include "jsobj.h"
#include "jsweakmap.h"

namespace js {

















struct ScopeCoordinate
{
    uint16_t hops;
    uint16_t binding;
    inline ScopeCoordinate(jsbytecode *pc);
};

inline JSAtom *
ScopeCoordinateAtom(JSScript *script, jsbytecode *pc);









































class ScopeObject : public JSObject
{
    
    void *getPrivate() const;

  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject &enclosingScope() const;
    inline bool setEnclosingScope(JSContext *cx, HandleObject obj);

    




    inline StackFrame *maybeStackFrame() const;
    inline void setStackFrame(StackFrame *frame);

    
    static inline size_t offsetOfEnclosingScope();
};

class CallObject : public ScopeObject
{
    static const uint32_t CALLEE_SLOT = 1;

    static CallObject *
    create(JSContext *cx, JSScript *script, HandleObject enclosing, HandleObject callee);

  public:
    static const uint32_t RESERVED_SLOTS = 3;

    static CallObject *createForFunction(JSContext *cx, StackFrame *fp);
    static CallObject *createForStrictEval(JSContext *cx, StackFrame *fp);

    
    inline bool isForEval() const;

    



    inline JSObject *getCallee() const;
    inline JSFunction *getCalleeFunction() const;
    inline void setCallee(JSObject *callee);

    
    inline const Value &arg(unsigned i) const;
    inline void setArg(unsigned i, const Value &v);
    inline void initArgUnchecked(unsigned i, const Value &v);

    
    inline const Value &var(unsigned i) const;
    inline void setVar(unsigned i, const Value &v);
    inline void initVarUnchecked(unsigned i, const Value &v);

    




    inline HeapSlotArray argArray();
    inline HeapSlotArray varArray();

    inline void copyValues(unsigned nargs, Value *argv, unsigned nvars, Value *slots);

    static JSBool getArgOp(JSContext *cx, HandleObject obj, HandleId id, Value *vp);
    static JSBool getVarOp(JSContext *cx, HandleObject obj, HandleId id, Value *vp);
    static JSBool setArgOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp);
    static JSBool setVarOp(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp);

    
    bool containsVarOrArg(PropertyName *name, Value *vp, JSContext *cx);
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
    
    js::StackFrame *maybeStackFrame() const;
    void setStackFrame(StackFrame *frame);

    static const unsigned THIS_SLOT = 2;

    
    JSObject *getProto() const;

  public:
    static const unsigned RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;

    static WithObject *
    create(JSContext *cx, HandleObject proto, HandleObject enclosing, uint32_t depth);

    
    JSObject &withThis() const;

    
    JSObject &object() const;
};

class BlockObject : public NestedScopeObject
{
  public:
    static const unsigned RESERVED_SLOTS = 2;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;

    
    inline uint32_t slotCount() const;

  protected:
    
    inline HeapSlot &slotValue(unsigned i);
};

class StaticBlockObject : public BlockObject
{
    
    StackFrame *maybeStackFrame() const;
    void setStackFrame(StackFrame *frame);

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

    



    bool needsClone() const;

    const Shape *addVar(JSContext *cx, jsid id, int index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, Handle<StaticBlockObject *> block,
                                     StackFrame *fp);

    
    StaticBlockObject &staticBlock() const;

    



    void put(StackFrame *fp);

    
    const Value &closedSlot(unsigned i);

    
    bool containsVar(PropertyName *name, Value *vp, JSContext *cx);
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
    static DebugScopeObject *create(JSContext *cx, ScopeObject &scope, JSObject &enclosing);

    ScopeObject &scope() const;
    JSObject &enclosingScope() const;

    
    bool isForDeclarative() const;
};


class DebugScopes
{
    
    typedef WeakMap<HeapPtrObject, HeapPtrObject> ObjectWeakMap;
    ObjectWeakMap proxiedScopes;

    



    typedef HashMap<ScopeIter, DebugScopeObject *, ScopeIter, RuntimeAllocPolicy> MissingScopeMap;
    MissingScopeMap missingScopes;

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

    



    void onPopCall(StackFrame *fp);
    void onPopBlock(JSContext *cx, StackFrame *fp);
    void onGeneratorFrameChange(StackFrame *from, StackFrame *to);
    void onCompartmentLeaveDebugMode(JSCompartment *c);
};

}  
#endif
