








































#ifndef jsion_codegen_x64_h__
#define jsion_codegen_x64_h__

#include "ion/x64/Assembler-x64.h"
#include "ion/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace ion {

class CodeGenerator : public CodeGeneratorX86Shared
{
  private:
    CodeGenerator *thisFromCtor() {
        return this;
    }

    Assembler masm;

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

    bool generatePrologue();
    bool generateEpilogue();

  public:
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
};

} 
} 

#endif 

