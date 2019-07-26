






#include "BaselineFrameInfo.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

bool
FrameInfo::init() {
    size_t nstack = script->nslots - script->nfixed;
    if (!stack.init(nstack))
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
      case StackValue::Register:
        masm.pushValue(val->reg());
        break;
      case StackValue::Constant:
        masm.pushValue(val->constant());
        break;
      default:
        JS_NOT_REACHED("Invalid kind");
        break;
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
      case StackValue::Stack:
        masm.popValue(dest);
        break;
      case StackValue::Register:
        masm.moveValue(val->reg(), dest);
        break;
      default:
        JS_NOT_REACHED("Invalid kind");
    }

    
    pop(false);
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
        JS_NOT_REACHED("Invalid uses");
    }
}

#ifdef DEBUG
void
FrameInfo::assertValidState(jsbytecode *pc)
{
    
    analyze::Bytecode *code = script->analysis()->maybeCode(pc);
    JS_ASSERT_IF(code, stackDepth() == code->stackDepth);

    
    uint32_t i = 0;
    for (; i < stackDepth(); i++) {
        if (stack[i].kind() != StackValue::Stack)
            break;
    }

    
    for (; i < stackDepth(); i++) {
        JS_ASSERT(stack[i].kind() != StackValue::Stack);
    }

    
    bool usedR0 = false, usedR1 = false, usedR2 = false;

    for (i = 0; i < stackDepth(); i++) {
        if (stack[i].kind() == StackValue::Register) {
            ValueOperand reg = stack[i].reg();
            if (reg == R0) {
                JS_ASSERT(!usedR0);
                usedR0 = true;
            } else if (reg == R1) {
                JS_ASSERT(!usedR1);
                usedR1 = true;
            } else if (reg == R2) {
                JS_ASSERT(!usedR2);
                usedR2 = true;
            } else {
                JS_NOT_REACHED("Invalid register");
            }
        }
    }
}
#endif
