








































#ifndef jsion_codegen_x64_h__
#define jsion_codegen_x64_h__

#include "ion/x64/Assembler-x64.h"
#include "ion/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX64 : public CodeGeneratorX86Shared
{
    CodeGeneratorX64 *thisFromCtor() {
        return this;
    }

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);

    
    Assembler::Condition testStringTruthy(bool truthy, const ValueOperand &value);

    void loadUnboxedValue(Operand source, MIRType type, const LDefinition *dest);
    void storeUnboxedValue(const LAllocation *value, MIRType valueType,
                           Operand dest, MIRType slotType);

  public:
    CodeGeneratorX64(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitValue(LValue *value);
    bool visitOsrValue(LOsrValue *value);
    bool visitBox(LBox *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitDouble(LDouble *ins);
    bool visitLoadSlotV(LLoadSlotV *ins);
    bool visitLoadSlotT(LLoadSlotT *load);
    bool visitStoreSlotT(LStoreSlotT *store);
    bool visitWriteBarrierV(LWriteBarrierV *barrier);
    bool visitWriteBarrierT(LWriteBarrierT *barrier);
    bool visitLoadElementV(LLoadElementV *load);
    bool visitLoadElementT(LLoadElementT *load);
    bool visitStoreElementV(LStoreElementV *store);
    bool visitStoreElementT(LStoreElementT *store);
    bool visitImplicitThis(LImplicitThis *lir);
};

typedef CodeGeneratorX64 CodeGeneratorSpecific;

} 
} 

#endif 

