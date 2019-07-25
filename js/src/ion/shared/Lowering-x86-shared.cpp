








































#include "ion/MIR.h"
#include "ion/Lowering.h"
#include "ion/shared/Lowering-x86-shared.h"
#include "ion/shared/Lowering-shared-inl.h"
using namespace js;
using namespace js::ion;

bool
LIRGeneratorX86Shared::visitTableSwitch(MTableSwitch *tableswitch)
{
    MDefinition *opd = tableswitch->getOperand(0);

    
    JS_ASSERT(tableswitch->numSuccessors() > 0);

    
    if (tableswitch->numSuccessors() == 1)
        return add(new LGoto(tableswitch->getDefault()));

    
    if (opd->type() != MIRType_Int32 && opd->type() != MIRType_Double)
        return add(new LGoto(tableswitch->getDefault()));

    
    
    LAllocation index;
    LDefinition tempInt;
    if (opd->type() == MIRType_Int32) {
        index = useCopy(opd);
        tempInt = LDefinition::BogusTemp();
    } else {
        index = useRegister(opd);
        tempInt = temp(LDefinition::INTEGER);
    }
    return add(new LTableSwitch(index, tempInt, temp(LDefinition::POINTER), tableswitch));
}
