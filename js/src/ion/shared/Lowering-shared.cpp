








































#include "ion/IonLIR.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "Lowering-shared.h"
#include "Lowering-shared-inl.h"

using namespace js;
using namespace ion;

bool
LIRGeneratorShared::visitConstant(MConstant *ins)
{
    const Value &v = ins->value();
    switch (ins->type()) {
      case MIRType_Boolean:
        return define(new LInteger(v.toBoolean()), ins);
      case MIRType_Int32:
        return define(new LInteger(v.toInt32()), ins);
      case MIRType_String:
        return define(new LPointer(v.toString()), ins);
      case MIRType_Object:
        return define(new LPointer(&v.toObject()), ins);
      default:
        
        
        JS_NOT_REACHED("unexpected constant type");
        return false;
    }
    return true;
}

bool
LIRGeneratorShared::defineTypedPhi(MPhi *phi, size_t lirIndex)
{
    LPhi *lir = current->getPhi(lirIndex);

    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setVirtualRegister(vreg);
    lir->setDef(0, LDefinition(vreg, LDefinition::TypeFrom(phi->type())));
    return annotate(lir);
}

void
LIRGeneratorShared::lowerTypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex)
{
    MDefinition *operand = phi->getOperand(inputPosition);
    LPhi *lir = block->getPhi(lirIndex);
    lir->setOperand(inputPosition, LUse(operand->virtualRegister(), LUse::ANY));
}

