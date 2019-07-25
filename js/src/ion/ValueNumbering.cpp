








































#include "Ion.h"
#include "ValueNumbering.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

ValueNumberer::ValueNumberer(MIRGraph &graph, bool optimistic)
  : graph_(graph),
    pessimisticPass_(!optimistic)
{ }


uint32
ValueNumberer::lookupValue(ValueMap &values, MDefinition *ins)
{

    ValueMap::AddPtr p = values.lookupForAdd(ins);

    if (!p) {
        if (!values.add(p, ins, ins->id()))
            return 0;
    }

    return p->value;
}

MDefinition *
ValueNumberer::simplify(MDefinition *def, bool useValueNumbers)
{
    if (!def->isIdempotent())
        return def;

    MDefinition *ins = def->foldsTo(useValueNumbers);

    if (ins == def)
        return def;

    if (!ins->block()) {
        
        

        
        JS_ASSERT(!def->isPhi());

        def->block()->insertAfter(def->toInstruction(), ins->toInstruction());
    }

    JS_ASSERT(ins->id() != 0);

    def->replaceAllUsesWith(ins);

    IonSpew(IonSpew_GVN, "Folding %d to be %d", def->id(), ins->id());
    return ins;
}




bool
ValueNumberer::computeValueNumbers()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_GVN, "Numbering instructions");

    ValueMap values;

    if (!values.init())
        return false;

    
    
    
    if (pessimisticPass_) {
        for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
            for (MDefinitionIterator iter(*block); iter; iter++)
                iter->setValueNumber(iter->id());
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
            for (MDefinitionIterator iter(*block); iter; ) {
                MDefinition *ins = simplify(*iter, false);

                if (ins != *iter) {
                    iter = block->removeDefAt(iter);
                    continue;
                }

                uint32 value = lookupValue(values, ins);

                if (!value)
                    return false; 

                if (ins->valueNumber() != value) {
                    IonSpew(IonSpew_GVN,
                            "Broke congruence for instruction %d (%p) with VN %d (now using %d)",
                            ins->id(), ins, ins->valueNumber(), value);
                    ins->setValueNumber(value);
                    changed = true;
                }

                iter++;
            }
        }
        values.clear();

        
        
        if (pessimisticPass_)
            break;
    }
    return true;
}

MDefinition *
ValueNumberer::findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index)
{
    InstructionMap::Ptr p = defs.lookup(ins->valueNumber());
    MDefinition *dom;
    if (!p || index > p->value.validUntil) {
        DominatingValue value;
        value.def = ins;
        
        
        
        
        
        
        
        value.validUntil = index + ins->block()->numDominated();

        if(!defs.put(ins->valueNumber(), value))
            return NULL;

        dom = ins;
    } else {
        dom = p->value.def;
    }

    return dom;
}

bool
ValueNumberer::eliminateRedundancies()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    InstructionMap defs;

    if (!defs.init())
        return false;

    IonSpew(IonSpew_GVN, "Eliminating redundant instructions");

    size_t index = 0;

    Vector<MBasicBlock *, 1, IonAllocPolicy> nodes;

    MBasicBlock *start = *graph_.begin();
    if (!nodes.append(start))
        return false;

    while (!nodes.empty()) {
        MBasicBlock *block = nodes.popCopy();

        IonSpew(IonSpew_GVN, "Looking at block %d", block->id());

        for (size_t i = 0; i < block->numImmediatelyDominatedBlocks(); i++) {
            if (!nodes.append(block->getImmediatelyDominatedBlock(i)))
                return false;
        }
        for (MDefinitionIterator iter(block); iter; ) {
            MDefinition *ins = simplify(*iter, true);

            if (ins != *iter) {
                iter = block->removeDefAt(iter);
                continue;
            }

            if (!ins->isIdempotent()) {
                iter++;
                continue;
            }

            MDefinition *dom = findDominatingDef(defs, ins, index);

            if (!dom)
                return false; 

            if (dom == ins) {
                iter++;
                continue;
            }

            IonSpew(IonSpew_GVN, "instruction %d is dominated by instruction %d (from block %d)",
                    ins->id(), dom->id(), dom->block()->id());

            ins->replaceAllUsesWith(dom);

            JS_ASSERT(!ins->hasUses());
            JS_ASSERT(ins->block() == block);
            iter = ins->block()->removeDefAt(iter);
        }
        index++;
    }

    JS_ASSERT(index == graph_.numBlocks());
    return true;
}


bool
ValueNumberer::analyze()
{
    return computeValueNumbers() && eliminateRedundancies();
}
