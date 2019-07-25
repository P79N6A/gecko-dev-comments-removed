







































#include "CodeGenerator-x64.h"
#include "ion/shared/CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::ion;

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph),
    moveHelper(thisFromCtor())
{
}

bool
CodeGenerator::visitMoveGroup(LMoveGroup *group)
{
    if (!moveGroupResolver.resolve(group))
        return false;

    moveHelper.setup(group);

    for (size_t i = 0; i < moveGroupResolver.numMoves(); i++)
        moveHelper.emit(moveGroupResolver.getMove(i));

    moveHelper.finish();
    return true;
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
    masm.cmpl(Imm32(JSVAL_TAG_INT32), ToOperand(result));
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
    masm.freeStack(frameDepth_);
    masm.ret();
    return true;
}

