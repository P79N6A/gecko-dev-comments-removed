








































#ifndef jsion_codegen_x64_h__
#define jsion_codegen_x64_h__

#include "ion/x64/Assembler-x64.h"
#include "ion/shared/CodeGenerator-x86-shared.h"
#include "ion/shared/MoveResolver-x86-shared.h"

namespace js {
namespace ion {

class CodeGenerator : public CodeGeneratorX86Shared
{
    friend class MoveResolverX86;

    MoveResolverX86 moveHelper;

    CodeGenerator *thisFromCtor() {
        return this;
    }

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitMoveGroup(LMoveGroup *group);
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
    bool visitBox(LBox *box);
    bool visitUnboxInteger(LUnboxInteger *unbox);
};

} 
} 

#endif 

