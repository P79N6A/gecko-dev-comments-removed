







































#include "CodeGenerator-x86.h"
#include "ion/shared/CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::ion;

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph, thisFromCtor()->masm)
{
}

bool
CodeGenerator::generatePrologue()
{
    return true;
}

bool
CodeGenerator::generateEpilogue()
{
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

bool
CodeGenerator::visitBox(LBox *box)
{
    LAllocation *a = box->getOperand(0);
    LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!a->isConstant());

    if (box->type() == MIRType_Double) {
        LDefinition *payload = box->getDef(PAYLOAD_INDEX);
        
    } else {
        
        
        
        JSValueTag tag;
        switch (box->type()) {
          case MIRType_Boolean:
            tag = JSVAL_TAG_BOOLEAN;
            break;
          case MIRType_Int32:
            tag = JSVAL_TAG_INT32;
            break;
          case MIRType_String:
            tag = JSVAL_TAG_STRING;
            break;
          case MIRType_Object:
            tag = JSVAL_TAG_OBJECT;
            break;
          default:
            JS_NOT_REACHED("no payload...");
            return false;
        }
        masm.movl(Imm32(tag), ToRegister(type));
    }
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
    masm.pop(ebp);
    masm.ret();
    return true;
}

