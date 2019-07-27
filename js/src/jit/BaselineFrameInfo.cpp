





#include "jit/BaselineFrameInfo.h"

#ifdef DEBUG
# include "jit/BytecodeAnalysis.h"
#endif

using namespace js;
using namespace js::jit;

bool
FrameInfo::init(TempAllocator &alloc)
{
    
    size_t nstack = Max(script->nslots() - script->nfixed(), size_t(1));
    if (!stack.init(alloc, nstack))
        return false;

    return true;
}

void
FrameInfo::sync(StackValue *val)
{
    switch (val->kind()) {
      case StackValue::Stack:
        break;
      case StackValue::LocalSlot:
        masm.pushValue(addressOfLocal(val->localSlot()));
        break;
      case StackValue::ArgSlot:
        masm.pushValue(addressOfArg(val->argSlot()));
        break;
      case StackValue::ThisSlot:
        masm.pushValue(addressOfThis());
        break;
      case StackValue::Register:
        masm.pushValue(val->reg());
        break;
      case StackValue::Constant:
        masm.pushValue(val->constant());
        break;
      default:
        MOZ_CRASH("Invalid kind");
    }

    val->setStack();
}

void
FrameInfo::syncStack(uint32_t uses)
{
    JS_ASSERT(uses <= stackDepth());

    uint32_t depth = stackDepth() - uses;

    for (uint32_t i = 0; i < depth; i++) {
        StackValue *current = &stack[i];
        sync(current);
    }
}

uint32_t
FrameInfo::numUnsyncedSlots()
{
    
    uint32_t i = 0;
    for (; i < stackDepth(); i++) {
        if (peek(-int32_t(i + 1))->kind() == StackValue::Stack)
            break;
    }
    return i;
}

void
FrameInfo::popValue(ValueOperand dest)
{
    StackValue *val = peek(-1);

    switch (val->kind()) {
      case StackValue::Constant:
        masm.moveValue(val->constant(), dest);
        break;
      case StackValue::LocalSlot:
        masm.loadValue(addressOfLocal(val->localSlot()), dest);
        break;
      case StackValue::ArgSlot:
        masm.loadValue(addressOfArg(val->argSlot()), dest);
        break;
      case StackValue::ThisSlot:
        masm.loadValue(addressOfThis(), dest);
        break;
      case StackValue::Stack:
        masm.popValue(dest);
        break;
      case StackValue::Register:
        masm.moveValue(val->reg(), dest);
        break;
      default:
        MOZ_CRASH("Invalid kind");
    }

    
    pop(DontAdjustStack);
}

void
FrameInfo::popRegsAndSync(uint32_t uses)
{
    
    
    
    JS_ASSERT(uses > 0);
    JS_ASSERT(uses <= 2);
    JS_ASSERT(uses <= stackDepth());

    syncStack(uses);

    switch (uses) {
      case 1:
        popValue(R0);
        break;
      case 2: {
        
        
        StackValue *val = peek(-2);
        if (val->kind() == StackValue::Register && val->reg() == R1) {
            masm.moveValue(R1, R2);
            val->setRegister(R2);
        }
        popValue(R1);
        popValue(R0);
        break;
      }
      default:
        MOZ_CRASH("Invalid uses");
    }
}

#ifdef DEBUG
void
FrameInfo::assertValidState(const BytecodeInfo &info)
{
    
    JS_ASSERT(stackDepth() == info.stackDepth);

    
    uint32_t i = 0;
    for (; i < stackDepth(); i++) {
        if (stack[i].kind() != StackValue::Stack)
            break;
    }

    
    for (; i < stackDepth(); i++)
        JS_ASSERT(stack[i].kind() != StackValue::Stack);

    
    
    
    bool usedR0 = false, usedR1 = false;

    for (i = 0; i < stackDepth(); i++) {
        if (stack[i].kind() == StackValue::Register) {
            ValueOperand reg = stack[i].reg();
            if (reg == R0) {
                JS_ASSERT(!usedR0);
                usedR0 = true;
            } else if (reg == R1) {
                JS_ASSERT(!usedR1);
                usedR1 = true;
            } else {
                MOZ_CRASH("Invalid register");
            }
        }
    }
}
#endif
