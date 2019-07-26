






#include "MoveResolver.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

MoveResolver::MoveResolver()
  : hasCycles_(false)
{
}

void
MoveResolver::resetState()
{
    hasCycles_ = false;
}

bool
MoveResolver::addMove(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    JS_ASSERT(!(from == to));
    PendingMove *pm = movePool_.allocate();
    if (!pm)
        return false;
    new (pm) PendingMove(from, to, kind);
    pending_.pushBack(pm);
    return true;
}



MoveResolver::PendingMove *
MoveResolver::findBlockingMove(const PendingMove *last)
{
    for (PendingMoveIterator iter = pending_.begin(); iter != pending_.end(); iter++) {
        PendingMove *other = *iter;

        if (other->from() == last->to()) {
            
            
            return other;
        }
    }

    
    return NULL;
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
                if (blocking->to() == pm->from()) {
                    
                    
                    
                    
                    pm->setInCycle();
                    blocking->setInCycle();
                    hasCycles_ = true;
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
    }

    return true;
}
