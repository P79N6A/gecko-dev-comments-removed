






#if !defined(jsion_baseline_frameinfo_h__) && defined(JS_ION)
#define jsion_baseline_frameinfo_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "BaselineJIT.h"
#include "BaselineRegisters.h"
#include "ion/IonMacroAssembler.h"
#include "FixedList.h"

namespace js {
namespace ion {



























class StackValue
{
  public:
    enum Kind {
        Constant,
        Register,
        Stack,
        LocalSlot,
        ArgSlot
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

  public:
    StackValue() {
        reset();
    }

    Kind kind() const {
        return kind_;
    }
    void reset() {
#ifdef DEBUG
        kind_ = Uninitialized;
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
    }
    void setRegister(const ValueOperand &val) {
        kind_ = Register;
        *data.reg.reg.addr() = val;
    }
    void setLocalSlot(uint32_t slot) {
        kind_ = LocalSlot;
        data.local.slot = slot;
    }
    void setArgSlot(uint32_t slot) {
        kind_ = ArgSlot;
        data.arg.slot = slot;
    }
    void setStack() {
        kind_ = Stack;
    }
};

enum StackAdjustment { AdjustStack, DontAdjustStack };

class FrameInfo
{
    JSContext *cx;
    JSScript *script;

    MacroAssembler &masm;

    FixedList<StackValue> stack;
    size_t spIndex;

  public:
    FrameInfo(JSContext *cx, JSScript *script, MacroAssembler &masm)
      : cx(cx),
        script(script),
        masm(masm),
        stack(),
        spIndex(0)
    { }

    bool init();

    uint32_t nlocals() const {
        return script->nfixed;
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
        for (uint32_t i = 0; i < n; i++)
            pop(adjust);
    }
    inline void push(const Value &val) {
        StackValue *sv = rawPush();
        sv->setConstant(val);
    }
    inline void push(const ValueOperand &val) {
        StackValue *sv = rawPush();
        sv->setRegister(val);
    }
    inline void pushLocal(uint32_t local) {
        StackValue *sv = rawPush();
        sv->setLocalSlot(local);
    }
    inline void pushArg(uint32_t arg) {
        StackValue *sv = rawPush();
        sv->setArgSlot(arg);
    }
    inline void pushScratchValue() {
        masm.pushValue(addressOfScratchValue());
        StackValue *sv = rawPush();
        sv->setStack();
    }
    inline Address addressOfLocal(size_t local) const {
        JS_ASSERT(local < nlocals());
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfLocal(local));
    }
    inline Address addressOfArg(size_t arg) const {
        JS_ASSERT(arg < nargs());
        return Address(BaselineFrameReg, BaselineFrame::offsetOfArg(arg));
    }
    inline Address addressOfStackValue(const StackValue *value) const {
        JS_ASSERT(value->kind() == StackValue::Stack);
        size_t slot = value - &stack[0];
        JS_ASSERT(slot < stackDepth());
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfLocal(nlocals() + slot));
    }
    inline Address addressOfScratchValue() const {
        return Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfScratchValue());
    }

    void popValue(ValueOperand dest);

    void sync(StackValue *val);
    void syncStack(uint32_t uses);
    void popRegsAndSync(uint32_t uses);

#ifdef DEBUG
    
    void assertValidState(jsbytecode *pc);
#else
    inline void assertValidState(jsbytecode *pc) {}
#endif
};

} 
} 

#endif

