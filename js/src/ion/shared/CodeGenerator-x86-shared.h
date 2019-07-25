








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    AssemblerX86Shared &masm;

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph, AssemblerX86Shared &masm);

  public:
    virtual bool visitLabel(LLabel *label);
    virtual bool visitMove(LMove *move);
    virtual bool visitGoto(LGoto *jump);
};

} 
} 

#endif 

