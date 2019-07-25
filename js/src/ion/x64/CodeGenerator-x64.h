








































#ifndef jsion_codegen_x64_h__
#define jsion_codegen_x64_h__

#include "ion/x64/Assembler-x64.h"
#include "ion/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace ion {

class CodeGenerator : public CodeGeneratorX86Shared
{
    CodeGenerator *thisFromCtor() {
        return this;
    }

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
    bool visitBox(LBox *box);
    bool visitUnboxInteger(LUnboxInteger *unbox);
    bool visitUnboxDouble(LUnboxDouble *unbox);
    bool visitDouble(LDouble *ins);
};

} 
} 

#endif 

