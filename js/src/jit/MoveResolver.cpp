





#include "jit/MoveResolver.h"
#include "jit/RegisterSets.h"

using namespace js;
using namespace js::jit;

MoveResolver::MoveResolver()
  : numCycles_(0), curCycles_(0)
{
}

void
MoveResolver::resetState()
{
    numCycles_ = 0;
    curCycles_ = 0;
}

bool
MoveResolver::addMove(const MoveOperand &from, const MoveOperand &to, MoveOp::Type type)
{
    
    JS_ASSERT(!(from == to));
    PendingMove *pm = movePool_.allocate();
    if (!pm)
        return false;
    new (pm) PendingMove(from, to, type);
    pending_.pushBack(pm);
    return true;
}



MoveResolver::PendingMove *
MoveResolver::findBlockingMove(const PendingMove *last)
{
    for (PendingMoveIterator iter = pending_.begin(); iter != pending_.end(); iter++) {
        PendingMove *other = *iter;

        if (other->from().aliases(last->to())) {
            
            
            return other;
        }
    }

    
    return nullptr;
}






MoveResolver::PendingMove *
MoveResolver::findCycledMove(PendingMoveIterator *iter, PendingMoveIterator end, const PendingMove *last)
{
    for (; *iter != end; (*iter)++) {
        PendingMove *other = **iter;
        if (other->from().aliases(last->to())) {
            
            
            (*iter)++;
            return other;
        }
    }
    
    return nullptr;
}

bool
MoveResolver::resolve()
{
    resetState();
    orderedMoves_.clear();

    InlineList<PendingMove> stack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    while (!pending_.empty()) {
        PendingMove *pm = pending_.popBack();

        
        stack.pushBack(pm);

        while (!stack.empty()) {
            PendingMove *blocking = findBlockingMove(stack.peekBack());

            if (blocking) {
                PendingMoveIterator stackiter = stack.begin();
                PendingMove *cycled = findCycledMove(&stackiter, stack.end(), blocking);
                if (cycled) {
                    
                    
                    
                    
                    
                    
                    do {
                        cycled->setCycleEnd(curCycles_);
                        cycled = findCycledMove(&stackiter, stack.end(), blocking);
                    } while (cycled);

                    blocking->setCycleBegin(pm->type(), curCycles_);
                    curCycles_++;
                    pending_.remove(blocking);
                    stack.pushBack(blocking);
                } else {
                    
                    
                    pending_.remove(blocking);
                    stack.pushBack(blocking);
                }
            } else {
                
                
                
                PendingMove *done = stack.popBack();
                if (!orderedMoves_.append(*done))
                    return false;
                movePool_.free(done);
            }
        }
        
        
        
        if (numCycles_ < curCycles_)
            numCycles_ = curCycles_;
        curCycles_ = 0;
    }

    return true;
}
