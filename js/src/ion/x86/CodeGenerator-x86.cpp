







































#include "CodeGenerator-x86.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"

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
    const LAllocation *a = box->getOperand(0);
    const LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!a->isConstant());

    
    
    
    masm.movl(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    return true;
}

bool
CodeGenerator::visitBoxDouble(LBoxDouble *box)
{
    const LDefinition *payload = box->getDef(PAYLOAD_INDEX);
    const LDefinition *type = box->getDef(TYPE_INDEX);
    const LDefinition *temp = box->getTemp(0);
    const LAllocation *in = box->getOperand(0);

    masm.movsd(ToFloatRegister(in), ToFloatRegister(temp));
    masm.movd(ToFloatRegister(temp), ToRegister(payload));
    masm.psrlq(Imm32(4), ToFloatRegister(temp));
    masm.movd(ToFloatRegister(temp), ToRegister(type));
    return true;
}

bool
CodeGenerator::visitUnbox(LUnbox *unbox)
{
    LAllocation *type = unbox->getOperand(TYPE_INDEX);
    masm.cmpl(ToOperand(type), Imm32(MIRTypeToTag(unbox->type())));
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
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jmp(returnLabel_);
    return true;
}

class DeferredDouble : public DeferredData
{
    double d_;

  public:
    DeferredDouble(double d) : d_(d)
    { }
    
    double d() const {
        return d_;
    }
    void copy(uint8 *code, uint8 *buffer) const {
        *(double *)buffer = d_;
    }
};

bool
CodeGenerator::visitDouble(LDouble *ins)
{
    const LDefinition *out = ins->getDef(0);

    jsdpun dpun;
    dpun.d = ins->getDouble();

    if (dpun.u64 == 0) {
        masm.xorpd(ToFloatRegister(out), ToFloatRegister(out));
        return true;
    }

    DeferredDouble *d = new DeferredDouble(ins->getDouble());
    if (!masm.addDeferredData(d, sizeof(double)))
        return false;

    masm.movsd(d->label(), ToFloatRegister(out));
    return true;
}

bool
CodeGenerator::visitUnboxDouble(LUnboxDouble *ins)
{
    const LAllocation *type = ins->getOperand(TYPE_INDEX);
    const LAllocation *payload = ins->getOperand(PAYLOAD_INDEX);
    const LDefinition *result = ins->getDef(0);
    const LDefinition *temp = ins->getTemp(0);

    masm.cmpl(ImmTag(JSVAL_TAG_CLEAR), ToRegister(type));
    masm.movd(ToRegister(payload), ToFloatRegister(result));
    masm.movd(ToRegister(type), ToFloatRegister(temp));
    masm.unpcklps(ToFloatRegister(temp), ToFloatRegister(result));
    return true;
}

bool
CodeGenerator::visitUnboxDoubleSSE41(LUnboxDoubleSSE41 *ins)
{
    const LAllocation *type = ins->getOperand(TYPE_INDEX);
    const LAllocation *payload = ins->getOperand(PAYLOAD_INDEX);
    const LDefinition *result = ins->getDef(0);

    masm.cmpl(ToOperand(type), ImmTag(JSVAL_TAG_CLEAR));
    masm.movd(ToRegister(payload), ToFloatRegister(result));
    masm.pinsrd(ToOperand(type), ToFloatRegister(result));
    return true;
}

