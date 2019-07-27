





#ifndef vm_GeneratorObject_h
#define vm_GeneratorObject_h

#include "jscntxt.h"
#include "jsobj.h"

#include "vm/ArgumentsObject.h"
#include "vm/ArrayObject.h"
#include "vm/Stack.h"

namespace js {

class GeneratorObject : public NativeObject
{
    static const int32_t MAX_BYTECODE_OFFSET = INT32_MAX >> 1;

  public:
    enum {
        CALLEE_SLOT = 0,
        THIS_SLOT,
        SCOPE_CHAIN_SLOT,
        ARGS_OBJ_SLOT,
        EXPRESSION_STACK_SLOT,
        BYTECODE_OFFSET_SLOT,
        RESERVED_SLOTS
    };

    enum SuspendKind { INITIAL, NORMAL, FINAL };
    enum ResumeKind { NEXT, THROW, CLOSE };

    static inline ResumeKind getResumeKind(jsbytecode *pc) {
        MOZ_ASSERT(*pc == JSOP_RESUME);
        unsigned arg = GET_UINT16(pc);
        MOZ_ASSERT(arg <= CLOSE);
        return static_cast<ResumeKind>(arg);
    }

    static inline ResumeKind getResumeKind(ExclusiveContext *cx, JSAtom *atom) {
        if (atom == cx->names().next)
            return NEXT;
        if (atom == cx->names().throw_)
            return THROW;
        MOZ_ASSERT(atom == cx->names().close);
        return CLOSE;
    }

    static JSObject *create(JSContext *cx, const InterpreterRegs &regs);

    static bool suspend(JSContext *cx, HandleObject obj, InterpreterFrame *fp, jsbytecode *pc,
                        Value *vp, unsigned nvalues, SuspendKind kind);

    static bool resume(JSContext *cx, InterpreterActivation &activation,
                       HandleObject obj, HandleValue arg, ResumeKind resumeKind);

    static bool initialSuspend(JSContext *cx, HandleObject obj, InterpreterFrame *fp, jsbytecode *pc) {
        return suspend(cx, obj, fp, pc, nullptr, 0, INITIAL);
    }

    static bool normalSuspend(JSContext *cx, HandleObject obj, InterpreterFrame *fp, jsbytecode *pc,
                              Value *vp, unsigned nvalues) {
        return suspend(cx, obj, fp, pc, vp, nvalues, NORMAL);
    }

    static bool finalSuspend(JSContext *cx, HandleObject obj);

    JSFunction &callee() const {
        return getFixedSlot(CALLEE_SLOT).toObject().as<JSFunction>();
    }
    void setCallee(JSFunction &callee) {
        setFixedSlot(CALLEE_SLOT, ObjectValue(callee));
    }

    const Value &thisValue() const {
        return getFixedSlot(THIS_SLOT);
    }
    void setThisValue(Value &thisv) {
        setFixedSlot(THIS_SLOT, thisv);
    }

    JSObject &scopeChain() const {
        return getFixedSlot(SCOPE_CHAIN_SLOT).toObject();
    }
    void setScopeChain(JSObject &scopeChain) {
        setFixedSlot(SCOPE_CHAIN_SLOT, ObjectValue(scopeChain));
    }

    bool hasArgsObj() const {
        return getFixedSlot(ARGS_OBJ_SLOT).isObject();
    }
    ArgumentsObject &argsObj() const {
        return getFixedSlot(ARGS_OBJ_SLOT).toObject().as<ArgumentsObject>();
    }
    void setArgsObj(ArgumentsObject &argsObj) {
        setFixedSlot(ARGS_OBJ_SLOT, ObjectValue(argsObj));
    }

    bool hasExpressionStack() const {
        return getFixedSlot(EXPRESSION_STACK_SLOT).isObject();
    }
    ArrayObject &expressionStack() const {
        return getFixedSlot(EXPRESSION_STACK_SLOT).toObject().as<ArrayObject>();
    }
    void setExpressionStack(ArrayObject &expressionStack) {
        setFixedSlot(EXPRESSION_STACK_SLOT, ObjectValue(expressionStack));
    }
    void clearExpressionStack() {
        setFixedSlot(EXPRESSION_STACK_SLOT, NullValue());
    }

    
    
    
    
    
    
    

    bool isRunning() const {
        MOZ_ASSERT(!isClosed());
        return getFixedSlot(BYTECODE_OFFSET_SLOT).toInt32() == 0;
    }
    bool isClosing() const {
        return getFixedSlot(BYTECODE_OFFSET_SLOT).toInt32() == MAX_BYTECODE_OFFSET << 1;
    }
    bool isSuspended() const {
        MOZ_ASSERT(!isClosed());
        
        return !isRunning() && !isClosing();
    }
    bool isNewborn() const {
        MOZ_ASSERT(!isClosed());
        return getFixedSlot(BYTECODE_OFFSET_SLOT).toInt32() & 0x1;
    }
    void setRunning() {
        MOZ_ASSERT(isSuspended());
        setFixedSlot(BYTECODE_OFFSET_SLOT, Int32Value(0));
    }
    void setClosing() {
        MOZ_ASSERT(isSuspended());
        setFixedSlot(BYTECODE_OFFSET_SLOT, Int32Value(MAX_BYTECODE_OFFSET << 1));
    }
    ptrdiff_t suspendedBytecodeOffset() const {
        MOZ_ASSERT(isSuspended());
        return getFixedSlot(BYTECODE_OFFSET_SLOT).toInt32() >> 1;
    }
    void setSuspendedBytecodeOffset(ptrdiff_t offset, bool newborn) {
        MOZ_ASSERT(newborn ? getFixedSlot(BYTECODE_OFFSET_SLOT).isUndefined() : isRunning());
        MOZ_ASSERT(offset > 0 && offset < MAX_BYTECODE_OFFSET);
        setFixedSlot(BYTECODE_OFFSET_SLOT, Int32Value((offset << 1) | (newborn ? 0x1 : 0)));
        MOZ_ASSERT(isSuspended());
    }
    bool isClosed() const {
        return getFixedSlot(CALLEE_SLOT).isNull();
    }
    void setClosed() {
        setFixedSlot(CALLEE_SLOT, NullValue());
        setFixedSlot(THIS_SLOT, NullValue());
        setFixedSlot(SCOPE_CHAIN_SLOT, NullValue());
        setFixedSlot(ARGS_OBJ_SLOT, NullValue());
        setFixedSlot(EXPRESSION_STACK_SLOT, NullValue());
        setFixedSlot(BYTECODE_OFFSET_SLOT, NullValue());
    }
};

class LegacyGeneratorObject : public GeneratorObject
{
  public:
    static const Class class_;

    static bool close(JSContext *cx, HandleObject obj);

    
    
    
    static bool maybeCloseNewborn(LegacyGeneratorObject *genObj) {
        if (genObj->isNewborn()) {
            genObj->setClosed();
            return true;
        }
        return false;
    }
};

class StarGeneratorObject : public GeneratorObject
{
  public:
    static const Class class_;
};

} 

template<>
inline bool
JSObject::is<js::GeneratorObject>() const
{
    return is<js::LegacyGeneratorObject>() || is<js::StarGeneratorObject>();
}

#endif 
