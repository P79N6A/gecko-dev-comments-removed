







































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

bool
CodeGeneratorX64::visitDouble(LDouble *ins)
{
    const LDefinition *temp = ins->getTemp(0);
    const LDefinition *out = ins->getDef(0);

    jsdpun dpun;
    dpun.d = ins->getDouble();

    if (dpun.u64 == 0) {
        masm.xorpd(ToFloatRegister(out), ToFloatRegister(out));
        return true;
    }

    masm.movq(ImmWord(dpun.u64), ToRegister(temp));
    masm.movqsd(ToRegister(temp), ToFloatRegister(out));
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
    const LAllocation *value = unbox->getOperand(0);
    const LDefinition *result = unbox->getDef(0);

    masm.movq(ToOperand(value), ToRegister(result));
    masm.shlq(Imm32(JSVAL_TAG_SHIFT), ToRegister(result));
    masm.cmpl(ToOperand(result), Imm32(JSVAL_TAG_INT32));
    masm.movl(ToOperand(value), ToRegister(result));

    return true;
}

bool
CodeGeneratorX64::visitUnboxDouble(LUnboxDouble *unbox)
{
    const LAllocation *value = unbox->getOperand(0);
    const LDefinition *result = unbox->getDef(0);
    const LDefinition *temp = unbox->getTemp(0);

    masm.movq(ImmWord(JSVAL_SHIFTED_TAG_MAX_DOUBLE), ToRegister(temp));
    masm.cmpq(ToRegister(value), ToRegister(temp));
    masm.movqsd(ToRegister(value), ToFloatRegister(result));

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

