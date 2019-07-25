








































#ifndef jsion_move_group_resolver_h__
#define jsion_move_group_resolver_h__

#include "IonLIR.h"

namespace js {
namespace ion {

class MoveGroupResolver
{
  public:
    class Move
    {
        const LMove *move_;
        bool cycle_;

      public:
        Move()
        { }
        Move(const LMove *move, bool cycle = false)
          : move_(move),
            cycle_(cycle)
        { }

        bool inCycle() const {
            return cycle_;
        }
        const LAllocation *from() const {
            return move_->from();
        }
        const LAllocation *to() const {
            return move_->to();
        }
    };

  private:
    struct PendingMove
      : public TempObject,
        public InlineListNode<PendingMove>
    {
        const LMove *move;
        bool cycle;

        PendingMove()
        { }

        PendingMove(const LMove *move)
          : move(move),
            cycle(false)
        { }
    };

  private:
    
    
    js::Vector<Move, 16, SystemAllocPolicy> unorderedMoves_;
    js::Vector<Move, 16, SystemAllocPolicy> orderedMoves_;
    bool hasCycles_;

    TempObjectPool<PendingMove> movePool_;

    InlineList<PendingMove> pending_;

    bool buildWorklist(LMoveGroup *group);
    PendingMove *findBlockingMove(const PendingMove *last);

  public:
    MoveGroupResolver();

    
    
    
    
    bool resolve(LMoveGroup *group);

    size_t numMoves() const {
        return orderedMoves_.length() + unorderedMoves_.length();
    }

    const Move &getMove(size_t i) const {
        if (i < orderedMoves_.length())
            return orderedMoves_[i];
        return unorderedMoves_[i - orderedMoves_.length()];
    }
    bool hasCycles() const {
        return hasCycles_;
    }
};

} 
} 

#endif 

