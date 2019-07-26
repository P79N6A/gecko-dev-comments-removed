





#include "jit/LICM.h"

#include <stdio.h>

#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

namespace {

typedef Vector<MBasicBlock*, 1, IonAllocPolicy> BlockQueue;
typedef Vector<MInstruction*, 1, IonAllocPolicy> InstructionQueue;

class Loop
{
    MIRGenerator *mir;

  public:
    
    enum LoopReturn {
        LoopReturn_Success,
        LoopReturn_Error, 
        LoopReturn_Skip   
    };

  public:
    
    Loop(MIRGenerator *mir, MBasicBlock *footer);

    
    LoopReturn init();

    
    bool optimize();

  private:
    
    MBasicBlock *header_;

    
    
    MBasicBlock* preLoop_;

    
    
    
    
    bool containsPossibleCall_;

    bool hoistInstructions(InstructionQueue &toHoist);

    
    bool isInLoop(MDefinition *ins);
    bool isBeforeLoop(MDefinition *ins);
    bool isLoopInvariant(MInstruction *ins);

    
    
    bool checkHotness(MBasicBlock *block);

    
    InstructionQueue worklist_;
    bool insertInWorklist(MInstruction *ins);
    MInstruction* popFromWorklist();

    inline bool isHoistable(const MDefinition *ins) const {
        return ins->isMovable() && !ins->isEffectful() && !ins->neverHoist();
    }

    bool requiresHoistedUse(const MDefinition *ins) const;
};

} 

LICM::LICM(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir), graph(graph)
{
}

bool
LICM::analyze()
{
    IonSpew(IonSpew_LICM, "Beginning LICM pass.");

    
    for (ReversePostorderIterator i(graph.rpoBegin()); i != graph.rpoEnd(); i++) {
        MBasicBlock *header = *i;

        
        if (!header->isLoopHeader() || header->numPredecessors() < 2)
            continue;

        
        Loop loop(mir, header);

        Loop::LoopReturn lr = loop.init();
        if (lr == Loop::LoopReturn_Error)
            return false;
        if (lr == Loop::LoopReturn_Skip) {
            graph.unmarkBlocks();
            continue;
        }

        if (!loop.optimize())
            return false;

        graph.unmarkBlocks();
    }

    return true;
}

Loop::Loop(MIRGenerator *mir, MBasicBlock *header)
  : mir(mir),
    header_(header),
    containsPossibleCall_(false)
{
    preLoop_ = header_->getPredecessor(0);
}

Loop::LoopReturn
Loop::init()
{
    IonSpew(IonSpew_LICM, "Loop identified, headed by block %d", header_->id());
    IonSpew(IonSpew_LICM, "footer is block %d", header_->backedge()->id());

    
    JS_ASSERT(header_->id() > header_->getPredecessor(0)->id());

    
    
    
    Vector<MBasicBlock *, 1, IonAllocPolicy> inlooplist;
    if (!inlooplist.append(header_->backedge()))
        return LoopReturn_Error;
    header_->backedge()->mark();

    while (!inlooplist.empty()) {
        MBasicBlock *block = inlooplist.back();

        
        
        
        
        
        
        if (block->immediateDominator() == block) {
            while (!worklist_.empty())
                popFromWorklist();
            return LoopReturn_Skip;
        }

        
        if (block != header_) {
            for (size_t i = 0; i < block->numPredecessors(); i++) {
                MBasicBlock *pred = block->getPredecessor(i);
                if (pred->isMarked())
                    continue;

                if (!inlooplist.append(pred))
                    return LoopReturn_Error;
                pred->mark();
            }
        }

        
        if (block != inlooplist.back())
            continue;

        
        for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
            MInstruction *ins = *i;

            
            
            if (ins->possiblyCalls())
                containsPossibleCall_ = true;

            if (isHoistable(ins)) {
                if (!insertInWorklist(ins))
                    return LoopReturn_Error;
            }
        }

        
        inlooplist.popBack();
    }

    return LoopReturn_Success;
}

bool
Loop::optimize()
{
    InstructionQueue invariantInstructions;

    IonSpew(IonSpew_LICM, "These instructions are in the loop: ");

    while (!worklist_.empty()) {
        if (mir->shouldCancel("LICM (worklist)"))
            return false;

        MInstruction *ins = popFromWorklist();

        IonSpewHeader(IonSpew_LICM);

        if (IonSpewEnabled(IonSpew_LICM)) {
            ins->printName(IonSpewFile);
            fprintf(IonSpewFile, " <- ");
            ins->printOpcode(IonSpewFile);
            fprintf(IonSpewFile, ":  ");
        }

        if (isLoopInvariant(ins)) {
            
            ins->setLoopInvariant();
            if (!invariantInstructions.append(ins))
                return false;

            if (IonSpewEnabled(IonSpew_LICM))
                fprintf(IonSpewFile, " Loop Invariant!\n");
        }
    }

    if (!hoistInstructions(invariantInstructions))
        return false;
    return true;
}

bool
Loop::requiresHoistedUse(const MDefinition *ins) const
{
    if (ins->isConstantElements() || ins->isBox())
        return true;

    
    
    
    
    if (ins->isConstant() && (IsFloatingPointType(ins->type()) || containsPossibleCall_))
        return true;

    return false;
}

bool
Loop::hoistInstructions(InstructionQueue &toHoist)
{
    
    for (int32_t i = toHoist.length() - 1; i >= 0; i--) {
        MInstruction *ins = toHoist[i];

        
        
        
        if (requiresHoistedUse(ins)) {
            bool loopInvariantUse = false;
            for (MUseDefIterator use(ins); use; use++) {
                if (use.def()->isLoopInvariant()) {
                    loopInvariantUse = true;
                    break;
                }
            }

            if (!loopInvariantUse)
                ins->setNotLoopInvariant();
        }
    }

    
    for (size_t i = 0; i < toHoist.length(); i++) {
        MInstruction *ins = toHoist[i];

        
        
        JS_ASSERT(!ins->isControlInstruction());
        JS_ASSERT(!ins->isEffectful());
        JS_ASSERT(ins->isMovable());

        if (!ins->isLoopInvariant())
            continue;

        if (checkHotness(ins->block())) {
            ins->block()->moveBefore(preLoop_->lastIns(), ins);
            ins->setNotLoopInvariant();
        }
    }

    return true;
}

bool
Loop::isInLoop(MDefinition *ins)
{
    return ins->block()->isMarked();
}

bool
Loop::isBeforeLoop(MDefinition *ins)
{
    return ins->block()->id() < header_->id();
}

bool
Loop::isLoopInvariant(MInstruction *ins)
{
    if (!isHoistable(ins)) {
        if (IonSpewEnabled(IonSpew_LICM))
            fprintf(IonSpewFile, "not hoistable\n");
        return false;
    }

    
    
    
    
    if (ins->dependency() && !isBeforeLoop(ins->dependency())) {
        if (IonSpewEnabled(IonSpew_LICM)) {
            fprintf(IonSpewFile, "depends on store inside or after loop: ");
            ins->dependency()->printName(IonSpewFile);
            fprintf(IonSpewFile, "\n");
        }
        return false;
    }

    
    
    for (size_t i = 0, e = ins->numOperands(); i < e; i++) {
        if (isInLoop(ins->getOperand(i)) &&
            !ins->getOperand(i)->isLoopInvariant()) {

            if (IonSpewEnabled(IonSpew_LICM)) {
                ins->getOperand(i)->printName(IonSpewFile);
                fprintf(IonSpewFile, " is in the loop.\n");
            }

            return false;
        }
    }

    return true;
}

bool
Loop::checkHotness(MBasicBlock *block)
{
    
    
    
    return true;
}

bool
Loop::insertInWorklist(MInstruction *ins)
{
    if (!worklist_.insert(worklist_.begin(), ins))
        return false;
    ins->setInWorklist();
    return true;
}

MInstruction*
Loop::popFromWorklist()
{
    MInstruction* toReturn = worklist_.popCopy();
    toReturn->setNotInWorklist();
    return toReturn;
}
