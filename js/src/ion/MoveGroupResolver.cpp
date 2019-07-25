







































#include "MoveGroupResolver.h"

using namespace js;
using namespace js::ion;

MoveGroupResolver::MoveGroupResolver()
  : hasCycles_(false)
{
}

typedef InlineList<MoveGroupResolver::PendingMove>::iterator PendingMoveIterator;

bool
MoveGroupResolver::buildWorklist(LMoveGroup *group)
{
    
    for (size_t i = 0; i < group->numMoves(); i++) {
        const LMove &move = group->getMove(i);

        
        JS_ASSERT(*move.from() != *move.to());

        
        if (move.from()->isConstant()) {
            if (!unorderedMoves_.append(Move(&move)))
                return false;
            continue;
        }

        PendingMove *pm = movePool_.allocate();
        new (pm) PendingMove(&move);
        pending_.insert(pm);
    }

    return true;
}



MoveGroupResolver::PendingMove *
MoveGroupResolver::findBlockingMove(const PendingMove *last)
{
    for (PendingMoveIterator iter = pending_.begin(); iter != pending_.end(); iter++) {
        PendingMove *other = *iter;

        if (*other->move->from() == *last->move->to()) {
            
            
            return other;
        }
    }

    
    return NULL;
}

bool
MoveGroupResolver::resolve(LMoveGroup *group)
{
    if (!buildWorklist(group))
        return false;

    unorderedMoves_.clear();
    orderedMoves_.clear();

    InlineList<PendingMove> stack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    while (!pending_.empty()) {
        PendingMove *pm = pending_.pop();

        
        stack.insert(pm);

        while (!stack.empty()) {
            PendingMove *blocking = findBlockingMove(stack.peekBack());

            if (blocking) {
                if (*blocking->move->to() == *pm->move->from()) {
                    
                    
                    
                    
                    JS_ASSERT(!pm->cycle);
                    if (!orderedMoves_.append(Move(blocking->move, true)));
                        return true;
                    pm->cycle = true;
                    hasCycles_ = true;
                    pending_.remove(blocking);
                    movePool_.free(blocking);
                } else {
                    
                    
                    pending_.remove(blocking);
                    stack.insert(blocking);
                }
            } else {
                
                
                
                PendingMove *done = stack.pop();
                if (!orderedMoves_.append(Move(done->move, done->cycle)))
                    return false;
                movePool_.free(done);
            }
        }
    }

    return true;
}
