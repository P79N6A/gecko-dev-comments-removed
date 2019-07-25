








































#ifndef ScopeObject_h___
#define ScopeObject_h___

#include "jsobj.h"

namespace js {





































class ScopeObject : public JSObject
{
    
    void *getPrivate() const;

  protected:
    static const uint32_t SCOPE_CHAIN_SLOT = 0;

  public:
    




    inline JSObject &enclosingScope() const;
    inline bool setEnclosingScope(JSContext *cx, JSObject &obj);

    



    inline StackFrame *maybeStackFrame() const;
    inline void setStackFrame(StackFrame *frame);

    
    static inline size_t offsetOfEnclosingScope();
};

class CallObject : public ScopeObject
{
    static const uint32_t CALLEE_SLOT = 1;
    static const uint32_t ARGUMENTS_SLOT = 2;

    static CallObject *
    create(JSContext *cx, JSScript *script, JSObject &enclosing, JSObject *callee);

  public:
    static const uint32_t RESERVED_SLOTS = 3;

    static CallObject *createForFunction(JSContext *cx, StackFrame *fp);
    static CallObject *createForStrictEval(JSContext *cx, StackFrame *fp);

    
    inline bool isForEval() const;

    



    inline JSObject *getCallee() const;
    inline JSFunction *getCalleeFunction() const;
    inline void setCallee(JSObject *callee);

    





    inline const Value &arguments() const;
    inline void setArguments(const Value &v);

    
    inline const Value &arg(uintN i) const;
    inline void setArg(uintN i, const Value &v);
    inline void initArgUnchecked(uintN i, const Value &v);

    
    inline const Value &var(uintN i) const;
    inline void setVar(uintN i, const Value &v);
    inline void initVarUnchecked(uintN i, const Value &v);

    




    inline HeapSlotArray argArray();
    inline HeapSlotArray varArray();

    inline void copyValues(uintN nargs, Value *argv, uintN nvars, Value *slots);

    static JSBool getArgumentsOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool setArgumentsOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
    static JSBool getArgOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool getVarOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool getUpvarOp(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool setArgOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
    static JSBool setVarOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
    static JSBool setUpvarOp(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
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
    create(JSContext *cx, StackFrame *fp, JSObject &proto, JSObject &enclosing, uint32_t depth);

    
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

    



    void setDefinitionParseNode(unsigned i, Definition *def);
    Definition *maybeDefinitionParseNode(unsigned i);
    void poisonDefinitionParseNode(unsigned i);

    const Shape *addVar(JSContext *cx, jsid id, intN index, bool *redeclared);
};

class ClonedBlockObject : public BlockObject
{
  public:
    static ClonedBlockObject *create(JSContext *cx, StaticBlockObject &block, StackFrame *fp);

    
    StaticBlockObject &staticBlock() const;

    



    void put(JSContext *cx);

    
    const Value &closedSlot(unsigned i);
};

extern bool
XDRStaticBlockObject(JSXDRState *xdr, JSScript *script, StaticBlockObject **objp);

}  

#endif 
