







































#include "MoveResolver.h"

using namespace js;
using namespace js::ion;

MoveResolver::MoveResolver()
  : hasCycles_(false)
{
}

bool
MoveResolver::addMove(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    PendingMove *pm = movePool_.allocate();
    if (!pm)
        return false;
    new (pm) PendingMove(from, to, kind);
    pending_.insert(pm);
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
    JS_ASSERT(!pending_.empty());

    orderedMoves_.clear();

    InlineList<PendingMove> stack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    while (!pending_.empty()) {
        PendingMove *pm = pending_.pop();

        
        stack.insert(pm);

        while (!stack.empty()) {
            PendingMove *blocking = findBlockingMove(stack.peekBack());

            if (blocking) {
                if (blocking->to() == pm->from()) {
                    
                    
                    
                    
                    pm->setInCycle();
                    blocking->setInCycle();
                    if (!orderedMoves_.append(*blocking))
                        return false;
                    hasCycles_ = true;
                    pending_.remove(blocking);
                    movePool_.free(blocking);
                } else {
                    
                    
                    pending_.remove(blocking);
                    stack.insert(blocking);
                }
            } else {
                
                
                
                PendingMove *done = stack.pop();
                if (!orderedMoves_.append(*done))
                    return false;
                movePool_.free(done);
            }
        }
    }

    return true;
}
