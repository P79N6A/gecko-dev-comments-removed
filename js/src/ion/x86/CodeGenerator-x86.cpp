







































#include "CodeGenerator-x86.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86::CodeGeneratorX86(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph)
{
}



static const uint32 LAST_FRAME_SIZE = 512;
static const uint32 LAST_FRAME_INCREMENT = 512;
static const uint32 FrameSizes[] = { 128, 256, LAST_FRAME_SIZE };

FrameSizeClass
FrameSizeClass::FromDepth(uint32 frameDepth)
{
    for (uint32 i = 0; i < JS_ARRAY_LENGTH(FrameSizes); i++) {
        if (frameDepth < FrameSizes[i])
            return FrameSizeClass(i);
    }

    uint32 newFrameSize = frameDepth - LAST_FRAME_SIZE;
    uint32 sizeClass = (newFrameSize / LAST_FRAME_INCREMENT) + 1;

    return FrameSizeClass(JS_ARRAY_LENGTH(FrameSizes) + sizeClass);
}
uint32
FrameSizeClass::frameSize() const
{
    JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);

    if (class_ < JS_ARRAY_LENGTH(FrameSizes))
        return FrameSizes[class_];

    uint32 step = class_ - JS_ARRAY_LENGTH(FrameSizes);
    return LAST_FRAME_SIZE + step * LAST_FRAME_INCREMENT;
}

bool
CodeGeneratorX86::visitValue(LValue *value)
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
CodeGeneratorX86::visitBox(LBox *box)
{
    const LAllocation *a = box->getOperand(0);
    const LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!a->isConstant());

    
    
    
    masm.movl(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    return true;
}

bool
CodeGeneratorX86::visitBoxDouble(LBoxDouble *box)
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
CodeGeneratorX86::visitUnbox(LUnbox *unbox)
{
    LAllocation *type = unbox->getOperand(TYPE_INDEX);
    masm.cmpl(ToOperand(type), Imm32(MIRTypeToTag(unbox->type())));
    return true;
}

bool
CodeGeneratorX86::visitReturn(LReturn *ret)
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
CodeGeneratorX86::visitDouble(LDouble *ins)
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
CodeGeneratorX86::visitUnboxDouble(LUnboxDouble *ins)
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
CodeGeneratorX86::visitUnboxDoubleSSE41(LUnboxDoubleSSE41 *ins)
{
    const LAllocation *type = ins->getOperand(TYPE_INDEX);
    const LAllocation *payload = ins->getOperand(PAYLOAD_INDEX);
    const LDefinition *result = ins->getDef(0);

    masm.cmpl(ToOperand(type), ImmTag(JSVAL_TAG_CLEAR));
    masm.movd(ToRegister(payload), ToFloatRegister(result));
    masm.pinsrd(ToOperand(type), ToFloatRegister(result));
    return true;
}

