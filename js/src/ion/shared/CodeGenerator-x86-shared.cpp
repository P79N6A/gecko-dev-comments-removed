







































#if defined(JS_CPU_X86)
# include "ion/x86/Assembler-x86.h"
#else
# include "ion/x64/Assembler-x64.h"
#endif
#include "CodeGenerator-x86-shared.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph, AssemblerX86Shared &masm)
  : CodeGeneratorShared(gen, graph),
    masm(masm)
{
}

bool
CodeGeneratorX86Shared::visitLabel(LLabel *label)
{
    masm.bind(label->label());
    return true;
}

bool
CodeGeneratorX86Shared::visitGoto(LGoto *jump)
{
    LBlock *target = jump->target()->lir();
    LLabel *header = target->begin()->toLabel();

    
    if (current->mir()->id() + 1 == target->mir()->id())
        return true;

    masm.jmp(header->label());
    return true;
}

bool
CodeGeneratorX86Shared::visitMove(LMove *move)
{
    return true;
}

