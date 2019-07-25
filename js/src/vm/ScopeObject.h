








































#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "jsobj.h"

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

    static JSBool getArgOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool getVarOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool setArgOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
    static JSBool setVarOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);

    
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
    static const unsigned THIS_SLOT = 2;

    
    JSObject *getProto() const;

  public:
    static const unsigned RESERVED_SLOTS = 3;
    static const gc::AllocKind FINALIZE_KIND = gc::FINALIZE_OBJECT4;

    static WithObject *
    create(JSContext *cx, StackFrame *fp, HandleObject proto, HandleObject enclosing, uint32_t depth);

    
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

    const Shape *addVar(JSContext *cx, jsid id, int index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, StaticBlockObject &block, StackFrame *fp);

    
    StaticBlockObject &staticBlock() const;

    



    void put(JSContext *cx);

    
    const Value &closedSlot(unsigned i);

    
    bool containsVar(PropertyName *name, Value *vp, JSContext *cx);
};

template<XDRMode mode>
bool
XDRStaticBlockObject(XDRState<mode> *xdr, JSScript *script, StaticBlockObject **objp);

extern JSObject *
CloneStaticBlockObject(JSContext *cx, StaticBlockObject &srcBlock,
                       const AutoObjectVector &objects, JSScript *src);

}  

#endif 
