





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
MoveResolver::addMove(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type)
{
    
    MOZ_ASSERT(!(from == to));
    PendingMove* pm = movePool_.allocate();
    if (!pm)
        return false;
    new (pm) PendingMove(from, to, type);
    pending_.pushBack(pm);
    return true;
}



MoveResolver::PendingMove*
MoveResolver::findBlockingMove(const PendingMove* last)
{
    for (PendingMoveIterator iter = pending_.begin(); iter != pending_.end(); iter++) {
        PendingMove* other = *iter;

        if (other->from().aliases(last->to())) {
            
            
            return other;
        }
    }

    
    return nullptr;
}






MoveResolver::PendingMove*
MoveResolver::findCycledMove(PendingMoveIterator* iter, PendingMoveIterator end, const PendingMove* last)
{
    for (; *iter != end; (*iter)++) {
        PendingMove* other = **iter;
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
        PendingMove* pm = pending_.popBack();

        
        stack.pushBack(pm);

        while (!stack.empty()) {
            PendingMove* blocking = findBlockingMove(stack.peekBack());

            if (blocking) {
                PendingMoveIterator stackiter = stack.begin();
                PendingMove* cycled = findCycledMove(&stackiter, stack.end(), blocking);
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
                
                
                
                PendingMove* done = stack.popBack();
                if (!addOrderedMove(*done))
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

bool
MoveResolver::addOrderedMove(const MoveOp& move)
{
    
    
    
    MOZ_ASSERT(!move.from().aliases(move.to()));

    if (!move.from().isMemory() || move.isCycleBegin() || move.isCycleEnd())
        return orderedMoves_.append(move);

    
    
    for (int i = orderedMoves_.length() - 1; i >= 0; i--) {
        const MoveOp& existing = orderedMoves_[i];

        if (existing.from() == move.from() &&
            !existing.to().aliases(move.to()) &&
            existing.type() == move.type() &&
            !existing.isCycleBegin() &&
            !existing.isCycleEnd())
        {
            MoveOp* after = orderedMoves_.begin() + i + 1;
            if (existing.to().isGeneralReg() || existing.to().isFloatReg()) {
                MoveOp nmove(existing.to(), move.to(), move.type());
                return orderedMoves_.insert(after, nmove);
            } else if (move.to().isGeneralReg() || move.to().isFloatReg()) {
                MoveOp nmove(move.to(), existing.to(), move.type());
                orderedMoves_[i] = move;
                return orderedMoves_.insert(after, nmove);
            }
        }

        if (existing.aliases(move))
            break;
    }

    return orderedMoves_.append(move);
}

void
MoveResolver::reorderMove(size_t from, size_t to)
{
    MOZ_ASSERT(from != to);

    MoveOp op = orderedMoves_[from];
    if (from < to) {
        for (size_t i = from; i < to; i++)
            orderedMoves_[i] = orderedMoves_[i + 1];
    } else {
        for (size_t i = from; i > to; i--)
            orderedMoves_[i] = orderedMoves_[i - 1];
    }
    orderedMoves_[to] = op;
}

void
MoveResolver::sortMemoryToMemoryMoves()
{
    
    
    
    
    for (size_t i = 0; i < orderedMoves_.length(); i++) {
        const MoveOp& base = orderedMoves_[i];
        if (!base.from().isMemory() || !base.to().isMemory())
            continue;
        if (base.type() != MoveOp::GENERAL && base.type() != MoveOp::INT32)
            continue;

        
        bool found = false;
        for (int j = i - 1; j >= 0; j--) {
            const MoveOp& previous = orderedMoves_[j];
            if (previous.aliases(base) || previous.isCycleBegin() || previous.isCycleEnd())
                break;

            if (previous.to().isGeneralReg()) {
                reorderMove(i, j);
                found = true;
                break;
            }
        }
        if (found)
            continue;

        
        if (i + 1 < orderedMoves_.length()) {
            bool found = false, skippedRegisterUse = false;
            for (size_t j = i + 1; j < orderedMoves_.length(); j++) {
                const MoveOp& later = orderedMoves_[j];
                if (later.aliases(base) || later.isCycleBegin() || later.isCycleEnd())
                    break;

                if (later.to().isGeneralReg()) {
                    if (skippedRegisterUse) {
                        reorderMove(i, j);
                        found = true;
                    } else {
                        
                        
                        
                        
                        
                    }
                    break;
                }

                if (later.from().isGeneralReg())
                    skippedRegisterUse = true;
            }

            if (found) {
                
                
                i--;
            }
        }
    }
}
