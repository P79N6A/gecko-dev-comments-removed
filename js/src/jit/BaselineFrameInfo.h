





#ifndef jit_BaselineFrameInfo_h
#define jit_BaselineFrameInfo_h

#ifdef JS_ION

#include "mozilla/Alignment.h"

#include "jit/BaselineFrame.h"
#include "jit/BaselineRegisters.h"
#include "jit/FixedList.h"
#include "jit/IonMacroAssembler.h"

namespace js {
namespace jit {

struct BytecodeInfo;



























class StackValue
{
  public:
    enum Kind {
        Constant,
        Register,
        Stack,
        LocalSlot,
        ArgSlot,
        ThisSlot
#ifdef DEBUG
        
        , Uninitialized
#endif
    };

  private:
    Kind kind_;

    union {
        struct {
            Value v;
        } constant;
        struct {
            mozilla::AlignedStorage2<ValueOperand> reg;
        } reg;
        struct {
            uint32_t slot;
        } local;
        struct {
            uint32_t slot;
        } arg;
    } data;

    JSValueType knownType_;

  public:
    StackValue() {
        reset();
    }

    Kind kind() const {
        return kind_;
    }
    bool hasKnownType() const {
        return knownType_ != JSVAL_TYPE_UNKNOWN;
    }
    bool hasKnownType(JSValueType type) const {
        JS_ASSERT(type != JSVAL_TYPE_UNKNOWN);
        return knownType_ == type;
    }
    bool isKnownBoolean() const {
        return hasKnownType(JSVAL_TYPE_BOOLEAN);
    }
    JSValueType knownType() const {
        JS_ASSERT(hasKnownType());
        return knownType_;
    }
    void reset() {
#ifdef DEBUG
        kind_ = Uninitialized;
        knownType_ = JSVAL_TYPE_UNKNOWN;
#endif
    }
    Value constant() const {
        JS_ASSERT(kind_ == Constant);
        return data.constant.v;
    }
    ValueOperand reg() const {
        JS_ASSERT(kind_ == Register);
        return *data.reg.reg.addr();
    }
    uint32_t localSlot() const {
        JS_ASSERT(kind_ == LocalSlot);
        return data.local.slot;
    }
    uint32_t argSlot() const {
        JS_ASSERT(kind_ == ArgSlot);
        return data.arg.slot;
    }

    void setConstant(const Value &v) {
        kind_ = Constant;
        data.constant.v = v;
        knownType_ = v.isDouble() ? JSVAL_TYPE_DOUBLE : v.extractNonDoubleType();
    }
    void setRegister(const ValueOperand &val, JSValueType knownType = JSVAL_TYPE_UNKNOWN) {
        kind_ = Register;
        *data.reg.reg.addr() = val;
        knownType_ = knownType;
    }
    void setLocalSlot(uint32_t slot) {
        kind_ = LocalSlot;
        data.local.slot = slot;
        knownType_ = JSVAL_TYPE_UNKNOWN;
    }
    void setArgSlot(uint32_t slot) {
        kind_ = ArgSlot;
        data.arg.slot = slot;
        knownType_ = JSVAL_TYPE_UNKNOWN;
    }
    void setThis() {
        kind_ = ThisSlot;
        knownType_ = JSVAL_TYPE_UNKNOWN;
    }
    void setStack() {
        kind_ = Stack;
        knownType_ = JSVAL_TYPE_UNKNOWN;
    }
};

enum StackAdjustment { AdjustStack, DontAdjustStack };

class FrameInfo
{
    RootedScript script;
    MacroAssembler &masm;

    FixedList<StackValue> stack;
    size_t spIndex;

  public:
    FrameInfo(JSContext *cx, HandleScript script, MacroAssembler &masm)
      : script(cx, script),
        masm(masm),
        stack(),
        spIndex(0)
    { }

    bool init(TempAllocator &alloc);

    uint32_t nlocals() const {
        return script->nfixed();
    }
    uint32_t nargs() const {
        return script->function()->nargs;
    }

  private:
    inline StackValue *rawPush() {
        StackValue *val = &stack[spIndex++];
        val->reset();
        return val;
    }

  public:
    inline size_t stackDepth() const {
        return spIndex;
    }
    inline void setStackDepth(uint32_t newDepth) {
        if (newDepth <= stackDepth()) {
            spIndex = newDepth;
        } else {
            uint32_t diff = newDepth - stackDepth();
            for (uint32_t i = 0; i < diff; i++) {
                StackValue *val = rawPush();
                val->setStack();
            }

            JS_ASSERT(spIndex == newDepth);
        }
    }
    inline StackValue *peek(int32_t index) const {
        JS_ASSERT(index < 0);
        return const_cast<StackValue *>(&stack[spIndex + index]);
    }

    inline void pop(StackAdjustment adjust = AdjustStack) {
        spIndex--;
        StackValue *popped = &stack[spIndex];

        if (adjust == AdjustStack && popped->kind() == StackValue::Stack)
            masm.addPtr(Imm32(sizeof(Value)), BaselineStackReg);

        
        popped->reset();
    }
    inline void popn(uint32_t n, StackAdjustment adjust = AdjustStack) {
        uint32_t poppedStack = 0;
        for (uint32_t i = 0; i < n; i++) {
            if (peek(-1)->kind() == StackValue::Stack)
                poppedStack++;
            pop(DontAdjustStack);
        }
        if (adjust == AdjustStack && poppedStack > 0)
            masm.addPtr(Imm32(sizeof(Value) * poppedStack), BaselineStackReg);
    }
    inline void push(const Value &val) {
        StackValue *sv = rawPush();
        sv->setConstant(val);
    }
    inline void push(const ValueOperand &val, JSValueType knownType=JSVAL_TYPE_UNKNOWN) {
        StackValue *sv = rawPush();
        sv->setRegister(val, knownType);
    }
    inline void pushLocal(uint32_t local) {
        StackValue *sv = rawPush();
        sv->setLocalSlot(local);
    }
    inline void pushArg(uint32_t arg) {
        StackValue *sv = rawPush();
        sv->setArgSlot(arg);
    }
    inline void pushThis() {
        StackValue *sv = rawPush();
        sv->setThis();
    }
    inline void pushScratchValue() {
        masm.pushValue(addressOfScratchValue());
        StackValue *sv = rawPush();
        sv->setStack();
    }
    inline Address addressOfLocal(size_t local) const {
#ifdef DEBUG
        if (local >= nlocals()) {
            
            
            size_t slot = local - nlocals();
            JS_ASSERT(slot < stackDepth());
            JS_ASSERT(stack[slot].kind() == StackValue::Stack);
        }
#endif
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfLocal(local));
    }
    Address addressOfArg(size_t arg) const {
        JS_ASSERT(arg < nargs());
        return Address(BaselineFrameReg, BaselineFrame::offsetOfArg(arg));
    }
    Address addressOfThis() const {
        return Address(BaselineFrameReg, BaselineFrame::offsetOfThis());
    }
    Address addressOfCallee() const {
        return Address(BaselineFrameReg, BaselineFrame::offsetOfCalleeToken());
    }
    Address addressOfScopeChain() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfScopeChain());
    }
    Address addressOfFlags() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFlags());
    }
    Address addressOfEvalScript() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfEvalScript());
    }
    Address addressOfReturnValue() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfReturnValue());
    }
    Address addressOfStackValue(const StackValue *value) const {
        JS_ASSERT(value->kind() == StackValue::Stack);
        size_t slot = value - &stack[0];
        JS_ASSERT(slot < stackDepth());
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfLocal(nlocals() + slot));
    }
    Address addressOfScratchValue() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfScratchValue());
    }

    void popValue(ValueOperand dest);

    void sync(StackValue *val);
    void syncStack(uint32_t uses);
    uint32_t numUnsyncedSlots();
    void popRegsAndSync(uint32_t uses);

    inline void assertSyncedStack() const {
        JS_ASSERT_IF(stackDepth() > 0, peek(-1)->kind() == StackValue::Stack);
    }

#ifdef DEBUG
    
    void assertValidState(const BytecodeInfo &info);
#else
    inline void assertValidState(const BytecodeInfo &info) {}
#endif
};

} 
} 

#endif 

#endif 
