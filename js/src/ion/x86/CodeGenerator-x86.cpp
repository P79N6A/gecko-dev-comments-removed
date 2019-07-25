







































#include "CodeGenerator-x86.h"
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

    LDefinition *type = value->getDef(TYPE_INDEX);
    LDefinition *payload = value->getDef(PAYLOAD_INDEX);

    masm.movl(Imm32(jv.s.tag), ToRegister(type));
    if (value->value().isMarkable())
        masm.movl(ImmGCPtr(jv.s.payload.ptr), ToRegister(payload));
    else
        masm.movl(Imm32(jv.s.payload.u32), ToRegister(payload));
    return true;
}

static inline JSValueTag 
MIRTypeToTag(MIRType type)
{
    switch (type) {
      case MIRType_Boolean:
        return JSVAL_TAG_BOOLEAN;
      case MIRType_Int32:
        return JSVAL_TAG_INT32;
      case MIRType_String:
        return JSVAL_TAG_STRING;
      case MIRType_Object:
        return JSVAL_TAG_OBJECT;
      default:
        JS_NOT_REACHED("no payload...");
    }
    return JSVAL_TAG_NULL;
}

bool
CodeGenerator::visitBox(LBox *box)
{
    LAllocation *a = box->getOperand(0);
    LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!a->isConstant());

    if (box->type() == MIRType_Double) {
        JS_NOT_REACHED("NYI");
    } else {
        
        
        
        masm.movl(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    }
    return true;
}

bool
CodeGenerator::visitUnbox(LUnbox *unbox)
{
    LAllocation *type = unbox->getOperand(TYPE_INDEX);
    masm.cmpl(Imm32(MIRTypeToTag(unbox->type())), ToOperand(type));
    return true;
}

bool
CodeGenerator::visitReturn(LReturn *ret)
{
#ifdef DEBUG
    LAllocation *type = ret->getOperand(TYPE_INDEX);
    LAllocation *payload = ret->getOperand(PAYLOAD_INDEX);

    JS_ASSERT(ToRegister(type) == JSReturnReg_Type);
    JS_ASSERT(ToRegister(payload) == JSReturnReg_Data);
#endif
    masm.freeStack(frameStaticSize_);
    masm.ret();
    return true;
}

