








































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

  public:
    CodeGeneratorX64(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
    bool visitStackArg(LStackArg *arg);
    bool visitBox(LBox *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitDouble(LDouble *ins);
    bool visitLoadSlotV(LLoadSlotV *ins);
    bool visitLoadSlotT(LLoadSlotT *load);
    bool visitGuardShape(LGuardShape *guard);
};

typedef CodeGeneratorX64 CodeGeneratorSpecific;

} 
} 

#endif 

