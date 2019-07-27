





#include "jit/LICM.h"

#include "jit/IonAnalysis.h"
#include "jit/JitSpewer.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;


static bool
LoopContainsPossibleCall(MIRGraph &graph, MBasicBlock *header, MBasicBlock *backedge)
{
    for (auto i(graph.rpoBegin(header)); ; ++i) {
        MOZ_ASSERT(i != graph.rpoEnd(), "Reached end of graph searching for blocks in loop");
        MBasicBlock *block = *i;
        if (!block->isMarked())
            continue;

        for (auto insIter(block->begin()), insEnd(block->end()); insIter != insEnd; ++insIter) {
            MInstruction *ins = *insIter;
            if (ins->possiblyCalls()) {
                JitSpew(JitSpew_LICM, "    Possile call found at %s%u", ins->opName(), ins->id());
                return true;
            }
        }

        if (block == backedge)
            break;
    }
    return false;
}








static bool
IsBeforeLoop(MDefinition *ins, MBasicBlock *header)
{
    return ins->block()->id() < header->id();
}



static bool
IsInLoop(MDefinition *ins)
{
    return ins->block()->isMarked();
}



static bool
RequiresHoistedUse(const MDefinition *ins, bool hasCalls)
{
    if (ins->isConstantElements())
        return true;

    if (ins->isBox()) {
        MOZ_ASSERT(!ins->toBox()->input()->isBox(),
                "Box of a box could lead to unbounded recursion");
        return true;
    }

    
    
    
    if (ins->isConstant() && (!IsFloatingPointType(ins->type()) || hasCalls))
        return true;

    return false;
}


static bool
HasOperandInLoop(MInstruction *ins, bool hasCalls)
{
    
    
    for (size_t i = 0, e = ins->numOperands(); i != e; ++i) {
        MDefinition *op = ins->getOperand(i);

        if (!IsInLoop(op))
            continue;

        if (RequiresHoistedUse(op, hasCalls)) {
            
            
            
            if (!HasOperandInLoop(op->toInstruction(), hasCalls))
                continue;
        }

        return true;
    }
    return false;
}



static bool
IsHoistableIgnoringDependency(MInstruction *ins, bool hasCalls)
{
    return ins->isMovable() && !ins->isEffectful() && !ins->neverHoist() &&
           !HasOperandInLoop(ins, hasCalls);
}


static bool
HasDependencyInLoop(MInstruction *ins, MBasicBlock *header)
{
    
    if (MDefinition *dep = ins->dependency())
        return !IsBeforeLoop(dep, header);
    return false;
}


static bool
IsHoistable(MInstruction *ins, MBasicBlock *header, bool hasCalls)
{
    return IsHoistableIgnoringDependency(ins, hasCalls) && !HasDependencyInLoop(ins, header);
}



static void
MoveDeferredOperands(MInstruction *ins, MInstruction *hoistPoint, bool hasCalls)
{
    
    
    for (size_t i = 0, e = ins->numOperands(); i != e; ++i) {
        MDefinition *op = ins->getOperand(i);
        if (!IsInLoop(op))
            continue;
        MOZ_ASSERT(RequiresHoistedUse(op, hasCalls),
                   "Deferred loop-invariant operand is not cheap");
        MInstruction *opIns = op->toInstruction();

        
        
        MoveDeferredOperands(opIns, hoistPoint, hasCalls);

        JitSpew(JitSpew_LICM, "    Hoisting %s%u (now that a user will be hoisted)",
                opIns->opName(), opIns->id());

        opIns->block()->moveBefore(hoistPoint, opIns);
    }
}

static void
VisitLoopBlock(MBasicBlock *block, MBasicBlock *header, MInstruction *hoistPoint, bool hasCalls)
{
    for (auto insIter(block->begin()), insEnd(block->end()); insIter != insEnd; ) {
        MInstruction *ins = *insIter++;

        if (!IsHoistable(ins, header, hasCalls)) {
#ifdef DEBUG
            if (IsHoistableIgnoringDependency(ins, hasCalls)) {
                JitSpew(JitSpew_LICM, "    %s%u isn't hoistable due to dependency on %s%u",
                        ins->opName(), ins->id(),
                        ins->dependency()->opName(), ins->dependency()->id());
            }
#endif
            continue;
        }

        
        
        
        if (RequiresHoistedUse(ins, hasCalls)) {
            JitSpew(JitSpew_LICM, "    %s%u will be hoisted only if its users are",
                    ins->opName(), ins->id());
            continue;
        }

        
        MoveDeferredOperands(ins, hoistPoint, hasCalls);

        JitSpew(JitSpew_LICM, "    Hoisting %s%u", ins->opName(), ins->id());

        
        block->moveBefore(hoistPoint, ins);
    }
}

static void
VisitLoop(MIRGraph &graph, MBasicBlock *header)
{
    MInstruction *hoistPoint = header->loopPredecessor()->lastIns();

    JitSpew(JitSpew_LICM, "  Visiting loop with header block%u, hoisting to %s%u",
            header->id(), hoistPoint->opName(), hoistPoint->id());

    MBasicBlock *backedge = header->backedge();

    
    
    
    
    bool hasCalls = LoopContainsPossibleCall(graph, header, backedge);

    for (auto i(graph.rpoBegin(header)); ; ++i) {
        MOZ_ASSERT(i != graph.rpoEnd(), "Reached end of graph searching for blocks in loop");
        MBasicBlock *block = *i;
        if (!block->isMarked())
            continue;

        VisitLoopBlock(block, header, hoistPoint, hasCalls);

        if (block == backedge)
            break;
    }
}

bool
jit::LICM(MIRGenerator *mir, MIRGraph &graph)
{
    JitSpew(JitSpew_LICM, "Beginning LICM pass");

    
    
    for (auto i(graph.rpoBegin()), e(graph.rpoEnd()); i != e; ++i) {
        MBasicBlock *header = *i;
        if (!header->isLoopHeader())
            continue;

        bool canOsr;
        MarkLoopBlocks(graph, header, &canOsr);

        
        
        
        if (!canOsr)
            VisitLoop(graph, header);
        else
            JitSpew(JitSpew_LICM, "  Skipping loop with header block%u due to OSR", header->id());

        UnmarkLoopBlocks(graph, header);

        if (mir->shouldCancel("LICM (main loop)"))
            return false;
    }

    return true;
}
