







































#include "CodeGenerator-x64.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"

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
    jsval_layout jv;
    jv.asBits = JSVAL_BITS(Jsvalify(value->value()));

    LDefinition *reg = value->getDef(0);

    if (value->value().isMarkable())
        masm.movq(ImmGCPtr(jv.asPtr), ToRegister(reg));
    else
        masm.movq(ImmWord(jv.asBits), ToRegister(reg));
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
CodeGeneratorX64::visitUnboxInteger(LUnboxInteger *unbox)
{
    const ValueOperand value = ToValue(unbox, LUnboxInteger::Input);
    const LDefinition *result = unbox->output();

    Assembler::Condition cond = masm.testInt32(Assembler::NotEqual, value);
    if (!bailoutIf(cond, unbox->snapshot()))
        return false;
    masm.unboxInt32(value, ToRegister(result));

    return true;
}

bool
CodeGeneratorX64::visitUnboxDouble(LUnboxDouble *unbox)
{
    const ValueOperand value = ToValue(unbox, LUnboxDouble::Input);
    const LDefinition *result = unbox->output();

    Assembler::Condition cond = masm.testDouble(Assembler::NotEqual, value);
    if (!bailoutIf(cond, unbox->snapshot()))
        return false;
    masm.unboxDouble(value, ToFloatRegister(result));

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

Register
CodeGeneratorX64::splitTagForTest(const ValueOperand &value)
{
    masm.splitTag(value, ScratchReg);
    return ScratchReg;
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
CodeGeneratorX64::visitCompareD(LCompareD *comp)
{
    JS_NOT_REACHED("Codegen for CompareD NYI");
    return false;
}

bool
CodeGeneratorX64::visitCompareDAndBranch(LCompareDAndBranch *comp)
{
    JS_NOT_REACHED("Codegen for CompareDAndBranch NYI");
    return false;
}
