








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;




























class TypeAnalyzer
{
    MIRGraph &graph;
    js::Vector<MInstruction *, 0, IonAllocPolicy> worklist;

  private:
    bool addToWorklist(MInstruction *ins);
    MInstruction *popFromWorklist();
    bool canSpecializeAtDef(MInstruction *ins);
    bool reflow(MInstruction *ins);

    bool populate();
    bool propagate();
    bool insertConversions();

    
    bool inspectUses(MInstruction *ins);
    bool inspectOperands(MInstruction *ins);
    bool propagateUsedTypes(MInstruction *ins);

    
    bool specializePhi(MPhi *phi);
    bool fixup(MInstruction *ins);
    void rewriteUses(MInstruction *old, MInstruction *ins);

  public:
    TypeAnalyzer(MIRGraph &graph);

    bool analyze();
};

TypeAnalyzer::TypeAnalyzer(MIRGraph &graph)
  : graph(graph)
{
}

bool
TypeAnalyzer::addToWorklist(MInstruction *ins)
{
    if (!ins->inWorklist()){
        ins->setInWorklist();
        return worklist.append(ins);
    }
    return true;
}

MInstruction *
TypeAnalyzer::popFromWorklist()
{
    MInstruction *ins = worklist.popCopy();
    ins->setNotInWorklist();
    return ins;
}

bool
TypeAnalyzer::populate()
{
    
    
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            if (!addToWorklist(block->getPhi(i)))
                return false;
        }
        MInstructionIterator i = block->begin();
        while (i != block->end()) {
            if (i->isCopy()) {
                
                MCopy *copy = i->toCopy();
                MUseIterator uses(copy);
                while (uses.more())
                    uses->ins()->replaceOperand(uses, copy->getInput(0));
                i = copy->block()->removeAt(i);
                continue;
            }
            addToWorklist(*i);
            i++;
        }
    }
    
    return true;
}


bool
TypeAnalyzer::inspectOperands(MInstruction *ins)
{
    for (size_t i = 0; i < ins->numOperands(); i++) {
        MIRType required = ins->requiredInputType(i);
        if (required >= MIRType_Value)
            continue;
        ins->getInput(i)->useAsType(required);
    }

    return true;
}



bool
TypeAnalyzer::inspectUses(MInstruction *ins)
{
    if (!ins->uses() || ins->type() == MIRType_None)
        return true;

    MIRType usedAs = ins->usedAsType();
    if (usedAs == MIRType_Value || ins->type() == usedAs)
        return true;

    
    
    
    MUseIterator uses(ins);
    while (uses.more()) {
        if (uses->ins()->adjustForInputs()) {
            if (!addToWorklist(uses->ins()))
                return false;
        }
        uses.next();
    }

    return true;
}

bool
TypeAnalyzer::propagateUsedTypes(MInstruction *ins)
{
    
    MPhi *phi = ins->toPhi();
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MInstruction *input = phi->getInput(i);
        bool changed = input->addUsedTypes(phi->usedTypes());
        if (changed && (input->isPhi() || ins->isCopy())) {
            
            
            
            if (!addToWorklist(input))
                return false;
        }
    }

    return true;
}

bool
TypeAnalyzer::propagate()
{
    
    while (!worklist.empty()) {
        MInstruction *ins = popFromWorklist();

        
        JS_ASSERT(!ins->isCopy());

        if (ins->isPhi()) {
            if (!propagateUsedTypes(ins))
                return false;
            if (!inspectUses(ins))
                return false;
        } else {
            if (!inspectUses(ins))
                return false;
            if (!inspectOperands(ins))
                return false;
        }
    }

    return true;
}




void
TypeAnalyzer::rewriteUses(MInstruction *old, MInstruction *ins)
{
    JS_ASSERT(old->type() == MIRType_Value && ins->type() < MIRType_Value);

    MUseIterator iter(old);
    while (iter.more()) {
        MInstruction *use = iter->ins();

        
        
        
        
        
        MIRType required = use->requiredInputType(iter->index());
        if ((required != MIRType_Any && required != ins->type()) || use->isSnapshot()) {
            if (use->isSnapshot())
                ins->rewritesDef();
            iter.next();
            continue;
        }

        use->replaceOperand(iter, ins);
    }
}

bool
TypeAnalyzer::specializePhi(MPhi *phi)
{
    
    
    MIRType usedAs = phi->usedAsType();
    if (usedAs == MIRType_Value)
        return false;

    
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MInstruction *ins = phi->getInput(i);
        if (ins->type() == usedAs)
            continue;

        
        
        if (ins->type() != MIRType_Value)
            return false;
    }

    return true;
}

bool
TypeAnalyzer::fixup(MInstruction *ins)
{
    if (!ins->uses())
        return true;

    
    MIRType usedAs = ins->usedAsType();
    if (usedAs != MIRType_Value && ins->type() == MIRType_Value) {
        MUnbox *unbox = MUnbox::New(ins, usedAs);
        MBasicBlock *block = ins->block();
        if (ins->isPhi()) {
            block->insertAfter(*block->begin(), unbox);
        } else if (block->start() && ins->id() < block->start()->id()) {
            block->insertAfter(block->start(), unbox);
        } else {
            block->insertAfter(ins, unbox);
        }
        rewriteUses(ins, unbox);
    }

    MUseIterator uses(ins);
    while (uses.more()) {
        MInstruction *use = uses->ins();
        MIRType required = use->requiredInputType(uses->index());
        MIRType actual = ins->type();

        if (required == actual || required == MIRType_Any) {
            
            uses.next();
            continue;
        }

        if (required == MIRType_Value) {
            
            MBox *box = MBox::New(ins);
            if (use->isPhi()) {
                MBasicBlock *pred = use->block()->getPredecessor(uses->index());
                pred->insertBefore(pred->lastIns(), box);
            } else {
                use->block()->insertBefore(use, box);
            }
            use->replaceOperand(uses, box);
        } else if (actual == MIRType_Value) {
            
            MUnbox *unbox = MUnbox::New(ins, required);
            use->block()->insertBefore(use, unbox);
            use->replaceOperand(uses, unbox);
        } else {
            
            JS_NOT_REACHED("NYI");
        }
    }

    return true;
}

bool
TypeAnalyzer::insertConversions()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i);
        for (size_t i = 0; i < block->numPhis(); i++) {
            
            if (!specializePhi(block->getPhi(i)))
                continue;
            if (!fixup(block->getPhi(i)))
                return false;
        }
        for (MInstructionIterator iter = block->begin(); iter != block->end(); iter++) {
            if (!fixup(*iter))
                return false;
        }
    }

    return true;
}

bool
TypeAnalyzer::analyze()
{
    if (!populate())
        return false;
    if (!propagate())
        return false;
    if (!insertConversions())
        return false;
    return true;
}

bool
ion::ApplyTypeInformation(MIRGraph &graph)
{
    TypeAnalyzer analysis(graph);
    if (!analysis.analyze())
        return false;
    return true;
}

bool
ion::ReorderBlocks(MIRGraph &graph)
{
    Vector<MBasicBlock *, 0, IonAllocPolicy> pending;
    Vector<unsigned int, 0, IonAllocPolicy> successors;
    Vector<MBasicBlock *, 0, IonAllocPolicy> done;

    MBasicBlock *current = graph.getBlock(0);
    unsigned int nextSuccessor = 0;

    graph.clearBlockList();

    
    while (true) {
        if (!current->isMarked()) {
            current->mark();

            if (nextSuccessor < current->lastIns()->numSuccessors()) {
                if (!pending.append(current))
                    return false;
                if (!successors.append(nextSuccessor))
                    return false;

                current = current->lastIns()->getSuccessor(nextSuccessor);
                nextSuccessor = 0;
                continue;
            }

            if (!done.append(current))
                return false;
        }

        if (pending.empty())
            break;

        current = pending.popCopy();
        current->unmark();
        nextSuccessor = successors.popCopy() + 1;
    }

    JS_ASSERT(pending.empty());
    JS_ASSERT(successors.empty());

    while (!done.empty()) {
        current = done.popCopy();
        current->unmark();
        if (!graph.addBlock(current))
            return false;
    }

    return true;
}

