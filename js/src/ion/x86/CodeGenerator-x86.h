








































#ifndef jsion_codegen_x86_h__
#define jsion_codegen_x86_h__

#include "Assembler-x86.h"
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
    bool visitBox(LBox *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
};

} 
} 

#endif 

