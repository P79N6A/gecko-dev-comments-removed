







































#include "CodeGenerator-x64.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"
#include "jsscope.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX64::CodeGeneratorX64(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph)
{
}

ValueOperand
CodeGeneratorX64::ToValue(LInstruction *ins, size_t pos)
{
    return ValueOperand(ToRegister(ins->getOperand(pos)));
}

bool
CodeGeneratorX64::visitDouble(LDouble *ins)
{
    const LDefinition *out = ins->output();
    masm.loadDouble(ins->getDouble(), ToFloatRegister(out));
    return true;
}

FrameSizeClass
FrameSizeClass::FromDepth(uint32 frameDepth)
{
    return FrameSizeClass::None();
}

uint32
FrameSizeClass::frameSize() const
{
    JS_NOT_REACHED("x64 does not use frame size classes");
    return 0;
}

bool
CodeGeneratorX64::visitValue(LValue *value)
{
    LDefinition *reg = value->getDef(0);
    masm.moveValue(value->value(), ToRegister(reg));
    return true;
}

static inline JSValueShiftedTag
MIRTypeToShiftedTag(MIRType type)
{
    switch (type) {
      case MIRType_Int32:
        return JSVAL_SHIFTED_TAG_INT32;
      case MIRType_String:
        return JSVAL_SHIFTED_TAG_STRING;
      case MIRType_Boolean:
        return JSVAL_SHIFTED_TAG_BOOLEAN;
      case MIRType_Object:
        return JSVAL_SHIFTED_TAG_OBJECT;
      default:
        JS_NOT_REACHED("unexpected type");
        return JSVAL_SHIFTED_TAG_NULL;
    }
}

bool
CodeGeneratorX64::visitBox(LBox *box)
{
    const LAllocation *in = box->getOperand(0);
    const LDefinition *result = box->getDef(0);

    if (box->type() != MIRType_Double) {
        JSValueShiftedTag tag = MIRTypeToShiftedTag(box->type());
        masm.movq(ImmWord(tag), ToRegister(result));
        masm.orq(ToOperand(in), ToRegister(result));
    } else {
        masm.movqsd(ToFloatRegister(in), ToRegister(result));
    }
    return true;
}

bool
CodeGeneratorX64::visitUnbox(LUnbox *unbox)
{
    const ValueOperand value = ToValue(unbox, LUnbox::Input);
    const LDefinition *result = unbox->output();
    MUnbox *mir = unbox->mir();

    if (mir->fallible()) {
        Assembler::Condition cond;
        switch (mir->type()) {
          case MIRType_Int32:
            cond = masm.testInt32(Assembler::NotEqual, value);
            break;
          case MIRType_Double:
            cond = masm.testDouble(Assembler::NotEqual, value);
            break;
          case MIRType_Object:
            cond = masm.testObject(Assembler::NotEqual, value);
            break;
          default:
            JS_NOT_REACHED("NYI");
            return false;
        }
        if (!bailoutIf(cond, unbox->snapshot()))
            return false;
    }

    switch (mir->type()) {
      case MIRType_Int32:
        masm.unboxInt32(value, ToRegister(result));
        break;
      case MIRType_Double:
        masm.unboxDouble(value, ToFloatRegister(result));
        break;
      case MIRType_Object:
        masm.unboxObject(value, ToRegister(result));
        break;
      default:
        JS_NOT_REACHED("NYI");
    }
    
    return true;
}

bool
CodeGeneratorX64::visitReturn(LReturn *ret)
{
#ifdef DEBUG
    LAllocation *result = ret->getOperand(0);
    JS_ASSERT(ToRegister(result) == JSReturnReg);
#endif
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jmp(returnLabel_);
    return true;
}

Assembler::Condition
CodeGeneratorX64::testStringTruthy(bool truthy, const ValueOperand &value)
{
    masm.unboxString(value, ScratchReg);

    Operand lengthAndFlags(ScratchReg, JSString::offsetOfLengthAndFlags());
    masm.movq(lengthAndFlags, ScratchReg);
    masm.shrq(Imm32(JSString::LENGTH_SHIFT), ScratchReg);
    masm.testq(ScratchReg, ScratchReg);
    return truthy ? Assembler::NonZero : Assembler::Zero;
}

bool
CodeGeneratorX64::visitLoadSlotV(LLoadSlotV *load)
{
    Register dest = ToRegister(load->outputValue());
    Register base = ToRegister(load->input());
    int32 offset = load->mir()->slot() * sizeof(js::Value);

    masm.movq(Operand(base, offset), dest);
    return true;
}

bool
CodeGeneratorX64::visitLoadSlotT(LLoadSlotT *load)
{
    Register base = ToRegister(load->input());
    int32 offset = load->mir()->slot() * sizeof(js::Value);

    switch (load->mir()->type()) {
      case MIRType_Double:
        masm.movsd(Operand(base, offset), ToFloatRegister(load->output()));
        break;
      case MIRType_Object:
      case MIRType_String:
      {
        Register out = ToRegister(load->output());
        masm.movq(Operand(base, offset), out);
        masm.unboxObject(ValueOperand(out), out);
        break;
      }
      case MIRType_Int32:
      case MIRType_Boolean:
        masm.movl(Operand(base, offset), ToRegister(load->output()));
        break;
      default:
        JS_NOT_REACHED("unexpected type");
        return false;
    }
    return true;
}

bool
CodeGeneratorX64::visitStackArg(LStackArg *arg)
{
    ValueOperand val = ToValue(arg, 0);
    uint32 argslot = arg->argslot();
    int32 stack_offset = StackOffsetOfPassedArg(argslot);

    masm.storeValue(val, Operand(StackPointer, stack_offset));
    return true;
}

bool
CodeGeneratorX64::visitGuardShape(LGuardShape *guard)
{
    Register obj = ToRegister(guard->input());
    masm.movq(ImmGCPtr(guard->mir()->shape()), ScratchReg);
    masm.cmpq(Operand(obj, offsetof(JSObject, lastProp)), ScratchReg);
    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
    return true;
}

