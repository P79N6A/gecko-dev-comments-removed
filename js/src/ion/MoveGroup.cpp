








































#include <cstdio>
#include "IonAllocPolicy.h"
#include "IonSpewer.h"
#include "MoveGroup.h"

using namespace js;
using namespace js::ion;

























bool
MoveGroup::toInstructionsBefore(LBlock *block, LInstruction *ins, uint32 stack)
{
#ifdef DEBUG
    
    for (Entry *i = entries_.begin(); i != entries_.end(); i++) {
        for (Entry *j = entries_.begin(); j != i; j++) {
            JS_ASSERT(*i->from != *j->from);
            JS_ASSERT(*i->to != *j->to);
        }
    }
#endif

    Vector<Entry, 0, IonAllocPolicy> workStack;
    while (!entries_.empty()) {
        bool done;
        Entry *currentPos = &entries_.back();
        do {
            Entry current = *currentPos;
            entries_.erase(currentPos);
            if (!workStack.append(current))
                return false;

            done = true;
            for (Entry *i = entries_.begin(); i != entries_.end(); i++) {
                if (*i->from == *current.to) {
                    done = false;
                    currentPos = i;
                    break;
                }
            }
        } while (!done);

        spewWorkStack(workStack);

        LAllocation temp;
        bool cycle = *workStack.back().to == *workStack.begin()->from;
        if (cycle) {
            
            
            bool isDouble = false;
            if (workStack[0].from->isFloatReg())
                isDouble = true;
            else if (workStack[0].from->isStackSlot())
                isDouble = workStack[0].from->toStackSlot()->isDouble();

            if (freeRegs.empty(isDouble)) {
                temp = LStackSlot(stack, isDouble);
            } else {
                if (isDouble)
                    temp = LFloatReg(freeRegs.takeAny(true).fpu());
                else
                    temp = LGeneralReg(freeRegs.takeAny(false).gpr());
            }
        }

        if (cycle)
            block->insertBefore(ins, new LMove(*workStack.back().to, temp));

        while (!workStack.empty()) {
            Entry *i = &workStack.back();
            if (cycle && i == workStack.begin())
                block->insertBefore(ins, new LMove(temp, *i->to));
            else
                block->insertBefore(ins, new LMove(*i->from, *i->to));
            workStack.erase(i);
        }
        if (cycle && (temp.isGeneralReg() || temp.isFloatReg()))
            freeRegs.add(temp.toRegister());
    }

    return true;
}

bool
MoveGroup::toInstructionsAfter(LBlock *block, LInstruction *ins, uint32 stack)
{
    LInstructionIterator iter(ins);
    iter++;
    return toInstructionsBefore(block, *iter, stack);
}

#ifdef DEBUG
void
MoveGroup::spewWorkStack(const Vector<Entry, 0, IonAllocPolicy>& workStack)
{
    IonSpew(IonSpew_LSRA, "  Resolving move chain:");
    for (Entry *i = workStack.begin(); i != workStack.end(); i++) {
        IonSpewHeader(IonSpew_LSRA);
        if (IonSpewEnabled(IonSpew_LSRA)) {
            fprintf(IonSpewFile, "   ");
            LAllocation::PrintAllocation(IonSpewFile, i->from);
            fprintf(IonSpewFile, " -> ");
            LAllocation::PrintAllocation(IonSpewFile, i->to);
            fprintf(IonSpewFile, "\n");
        }
    }
}
#endif
