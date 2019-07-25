







































#include "CodeGenerator-x64.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"

using namespace js;
using namespace js::ion;

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph)
{
}

bool
CodeGenerator::visitValue(LValue *value)
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
CodeGenerator::visitBox(LBox *box)
{
    const LAllocation *in = box->getOperand(0);
    const LDefinition *result = box->getDef(0);

    if (box->type() != MIRType_Double) {
        JSValueShiftedTag tag = MIRTypeToShiftedTag(box->type());
        masm.movq(ImmWord(tag), ToRegister(result));
        masm.orq(ToOperand(in), ToRegister(result));
    } else {
        JS_NOT_REACHED("NYI");
    }
    return true;
}

bool
CodeGenerator::visitUnboxInteger(LUnboxInteger *unbox)
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
CodeGenerator::visitReturn(LReturn *ret)
{
#ifdef DEBUG
    LAllocation *result = ret->getOperand(0);
    JS_ASSERT(ToRegister(result) == JSReturnReg);
#endif
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jmp(returnLabel_);
    return true;
}

