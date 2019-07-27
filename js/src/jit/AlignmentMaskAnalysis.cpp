





#include "jit/AlignmentMaskAnalysis.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace jit;

static bool
IsAlignmentMask(uint32_t m)
{
    
    return (-m & ~m) == 0;
}

static void
AnalyzeAsmHeapAddress(MDefinition *ptr, MIRGraph &graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (!ptr->isBitAnd())
        return;

    MDefinition *lhs = ptr->toBitAnd()->getOperand(0);
    MDefinition *rhs = ptr->toBitAnd()->getOperand(1);
    if (lhs->isConstantValue())
        mozilla::Swap(lhs, rhs);
    if (!lhs->isAdd() || !rhs->isConstantValue())
        return;

    MDefinition *op0 = lhs->toAdd()->getOperand(0);
    MDefinition *op1 = lhs->toAdd()->getOperand(1);
    if (op0->isConstantValue())
        mozilla::Swap(op0, op1);
    if (!op1->isConstantValue())
        return;

    uint32_t i = op1->constantValue().toInt32();
    uint32_t m = rhs->constantValue().toInt32();
    if (!IsAlignmentMask(m) || (i & m) != i)
        return;

    
    MInstruction *and_ = MBitAnd::NewAsmJS(graph.alloc(), op0, rhs);
    ptr->block()->insertBefore(ptr->toBitAnd(), and_);
    MInstruction *add = MAdd::NewAsmJS(graph.alloc(), and_, op1, MIRType_Int32);
    ptr->block()->insertBefore(ptr->toBitAnd(), add);
    ptr->replaceAllUsesWith(add);
    ptr->block()->discard(ptr->toBitAnd());
}

bool
AlignmentMaskAnalysis::analyze()
{
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
            
            
            
            if (i->isAsmJSLoadHeap())
                AnalyzeAsmHeapAddress(i->toAsmJSLoadHeap()->ptr(), graph_);
            else if (i->isAsmJSStoreHeap())
                AnalyzeAsmHeapAddress(i->toAsmJSStoreHeap()->ptr(), graph_);
        }
    }
    return true;
}
