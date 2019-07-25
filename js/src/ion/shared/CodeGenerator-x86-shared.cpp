







































#if defined(JS_CPU_X86)
# include "ion/x86/Assembler-x86.h"
#else
# include "ion/x64/Assembler-x64.h"
#endif
#include "CodeGenerator-x86-shared.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph, AssemblerX86Shared &masm)
  : CodeGeneratorShared(gen, graph),
    masm(masm)
{
    stackDepth_ = graph.stackHeight();
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

bool
CodeGeneratorX86Shared::visitAddI(LAddI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    if (rhs->isConstant())
        masm.addl(Imm32(ToInt32(rhs)), ToOperand(lhs));
    else
        masm.addl(ToOperand(rhs), ToRegister(lhs));

    return true;
}

bool
CodeGeneratorX86Shared::visitBitOp(LBitOp *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    switch (ins->bitop()) {
        case JSOP_BITOR:
            if (rhs->isConstant())
                masm.orl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.orl(ToOperand(rhs), ToRegister(lhs));
            break;
        case JSOP_BITXOR:
            if (rhs->isConstant())
                masm.xorl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.xorl(ToOperand(rhs), ToRegister(lhs));
            break;
        case JSOP_BITAND:
            if (rhs->isConstant())
                masm.andl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.andl(ToOperand(rhs), ToRegister(lhs));
            break;
        default:
            JS_NOT_REACHED("unexpected binary opcode");
    }

    return true;
}

