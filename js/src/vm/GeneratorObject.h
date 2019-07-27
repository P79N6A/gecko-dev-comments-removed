





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
        YIELD_INDEX_SLOT,
        RESERVED_SLOTS
    };

    enum ResumeKind { NEXT, THROW, CLOSE };

  private:
    static bool suspend(JSContext *cx, HandleObject obj, AbstractFramePtr frame, jsbytecode *pc,
                        Value *vp, unsigned nvalues);

  public:
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

    static JSObject *create(JSContext *cx, AbstractFramePtr frame);

    static bool resume(JSContext *cx, InterpreterActivation &activation,
                       HandleObject obj, HandleValue arg, ResumeKind resumeKind);

    static bool initialSuspend(JSContext *cx, HandleObject obj, AbstractFramePtr frame, jsbytecode *pc) {
        return suspend(cx, obj, frame, pc, nullptr, 0);
    }

    static bool normalSuspend(JSContext *cx, HandleObject obj, AbstractFramePtr frame, jsbytecode *pc,
                              Value *vp, unsigned nvalues) {
        return suspend(cx, obj, frame, pc, vp, nvalues);
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
    void setSuspendedBytecodeOffset(ptrdiff_t offset, uint32_t yieldIndex) {
        bool newborn = (yieldIndex == 0);
        MOZ_ASSERT(newborn ? getFixedSlot(BYTECODE_OFFSET_SLOT).isUndefined() : isRunning());
        MOZ_ASSERT(offset > 0 && offset < MAX_BYTECODE_OFFSET);
        setFixedSlot(BYTECODE_OFFSET_SLOT, Int32Value((offset << 1) | (newborn ? 0x1 : 0)));
        MOZ_ASSERT(isSuspended());
        MOZ_ASSERT(yieldIndex <= INT32_MAX);
        setFixedSlot(YIELD_INDEX_SLOT, Int32Value(yieldIndex));
    }

    
    
    
    
    uint32_t suspendedYieldIndex() const {
        MOZ_ASSERT(isSuspended());
        return getFixedSlot(YIELD_INDEX_SLOT).toInt32();
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
        setFixedSlot(YIELD_INDEX_SLOT, NullValue());
    }

    static size_t offsetOfCalleeSlot() {
        return getFixedSlotOffset(CALLEE_SLOT);
    }
    static size_t offsetOfThisSlot() {
        return getFixedSlotOffset(THIS_SLOT);
    }
    static size_t offsetOfScopeChainSlot() {
        return getFixedSlotOffset(SCOPE_CHAIN_SLOT);
    }
    static size_t offsetOfArgsObjSlot() {
        return getFixedSlotOffset(ARGS_OBJ_SLOT);
    }
    static size_t offsetOfBytecodeOffsetSlot() {
        return getFixedSlotOffset(BYTECODE_OFFSET_SLOT);
    }
    static size_t offsetOfYieldIndexSlot() {
        return getFixedSlotOffset(YIELD_INDEX_SLOT);
    }
    static size_t offsetOfExpressionStackSlot() {
        return getFixedSlotOffset(EXPRESSION_STACK_SLOT);
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

bool GeneratorThrow(JSContext *cx, HandleObject obj, HandleValue val);

} 

template<>
inline bool
JSObject::is<js::GeneratorObject>() const
{
    return is<js::LegacyGeneratorObject>() || is<js::StarGeneratorObject>();
}

#endif 
