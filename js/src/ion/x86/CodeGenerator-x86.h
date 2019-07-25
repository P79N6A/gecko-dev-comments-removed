








































#ifndef jsion_codegen_x86_h__
#define jsion_codegen_x86_h__

#include "Assembler-x86.h"
#include "ion/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX86 : public CodeGeneratorX86Shared
{
    CodeGeneratorX86 *thisFromCtor() {
        return this;
    }

  public:
    CodeGeneratorX86(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitBox(LBox *box);
    bool visitBoxDouble(LBoxDouble *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitUnboxDouble(LUnboxDouble *ins);
    bool visitUnboxDoubleSSE41(LUnboxDoubleSSE41 *ins);
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
    bool visitDouble(LDouble *ins);
};

typedef CodeGeneratorX86 CodeGeneratorSpecific;

} 
} 

#endif 

