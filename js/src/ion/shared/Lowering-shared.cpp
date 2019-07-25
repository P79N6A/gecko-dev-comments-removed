








































#include "ion/IonLIR.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "Lowering-shared.h"
#include "Lowering-shared-inl.h"

using namespace js;
using namespace ion;

bool
LIRGeneratorShared::lowerPhi(MPhi *ins)
{
    
    JS_ASSERT(ins->id());

    LPhi *phi = LPhi::New(gen, ins);
    if (!phi)
        return false;
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MDefinition *opd = ins->getOperand(i);
        JS_ASSERT(opd->type() == ins->type());

        phi->setOperand(i, LUse(opd->id(), LUse::ANY));
    }
    phi->setDef(0, LDefinition(ins->id(), LDefinition::TypeFrom(ins->type())));
    return addPhi(phi);
}

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

