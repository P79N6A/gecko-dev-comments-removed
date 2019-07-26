






#if !defined(jsion_baseline_frameinfo_h__) && defined(JS_ION)
#define jsion_baseline_frameinfo_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "BaselineJIT.h"
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
        LocalSlot
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
            AlignedStorage2<ValueOperand> reg;
        } reg;
        struct {
            uint32_t slot;
        } local;
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
    void setStack() {
        kind_ = Stack;
    }
};

static const Register frameReg = ebp;
static const Register spReg = StackPointer;

static const ValueOperand R0(ecx, edx);
static const ValueOperand R1(eax, ebx);
static const ValueOperand R2(esi, edi);

class BasicFrame
{
    uint32_t dummy1;
    uint32_t dummy2;

  public:
    static inline size_t offsetOfLocal(unsigned index) {
        return sizeof(BasicFrame) + index * sizeof(Value);
    }
};

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

  private:
    inline StackValue *rawPush() {
        StackValue *val = &stack[spIndex++];
        val->reset();
        return val;
    }

    uint32_t nlocals() const {
        return script->nfixed;
    }

  public:
    inline size_t stackDepth() const {
        return spIndex;
    }
    inline StackValue *peek(int32_t index) const {
        JS_ASSERT(index < 0);
        return const_cast<StackValue *>(&stack[spIndex + index]);
    }
    inline void pop(bool adjustStack = true) {
        spIndex--;
        StackValue *popped = &stack[spIndex];

        if (adjustStack && popped->kind() == StackValue::Stack)
            masm.addPtr(Imm32(sizeof(Value)), spReg);

        
        popped->reset();
    }
    inline void popn(uint32_t n) {
        for (uint32_t i = 0; i < n; i++)
            pop();
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
    inline Address addressOfLocal(size_t local) const {
        JS_ASSERT(local < nlocals());
        return Address(frameReg, -BasicFrame::offsetOfLocal(local));
    }
    inline Address addressOfStackValue(const StackValue *value) const {
        JS_ASSERT(value->kind() == StackValue::Stack);
        size_t slot = value - &stack[0];
        JS_ASSERT(slot < stackDepth());
        return Address(frameReg, -BasicFrame::offsetOfLocal(nlocals() + slot));
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

