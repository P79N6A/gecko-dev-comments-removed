








































#ifndef jsion_move_group_resolver_h__
#define jsion_move_group_resolver_h__

#include "IonRegisters.h"
#include "InlineList.h"
#include "IonAllocPolicy.h"

namespace js {
namespace ion {

class MoveResolver
{
  public:
    
    
    class MoveOperand
    {
        enum Kind {
            REG,
            FLOAT_REG,
            ADDRESS
        };

        Kind kind_;
        uint32 code_;
        int32 disp_;

      public:
        MoveOperand()
        { }
        explicit MoveOperand(const Register &reg) : kind_(REG), code_(reg.code())
        { }
        explicit MoveOperand(const FloatRegister &reg) : kind_(FLOAT_REG), code_(reg.code())
        { }
        MoveOperand(const Register &reg, int32 disp)
          : kind_(ADDRESS),
            code_(reg.code()),
            disp_(disp)
        { }
        MoveOperand(const MoveOperand &other)
          : kind_(other.kind_),
            code_(other.code_),
            disp_(other.disp_)
        { }
        bool isFloatReg() const {
            return kind_ == FLOAT_REG;
        }
        bool isGeneralReg() const {
            return kind_ == REG;
        }
        bool isDouble() const {
            return kind_ == FLOAT_REG;
        }
        bool isMemory() const {
            return kind_ == ADDRESS;
        }
        Register reg() const {
            JS_ASSERT(isGeneralReg());
            return Register::FromCode(code_);
        }
        FloatRegister floatReg() const {
            JS_ASSERT(isFloatReg());
            return FloatRegister::FromCode(code_);
        }
        Register base() const {
            JS_ASSERT(isMemory());
            return Register::FromCode(code_);
        }
        int32 disp() const {
            return disp_;
        }

        bool operator ==(const MoveOperand &other) const {
            if (kind_ != other.kind_)
                return false;
            if (code_ != other.code_)
                return false;
            if (isMemory())
                return disp_ == other.disp_;
            return true;
        }
    };

    class Move
    {
      protected:
        MoveOperand from_;
        MoveOperand to_;
        bool cycle_;

      public:
        enum Kind {
            GENERAL,
            DOUBLE
        };

      protected:
        Kind kind_;

      public:
        Move()
        { }
        Move(const Move &other)
          : from_(other.from_),
            to_(other.to_),
            cycle_(other.cycle_),
            kind_(other.kind_)
        { }
        Move(const MoveOperand &from, const MoveOperand &to, Kind kind, bool cycle = false)
          : from_(from),
            to_(to),
            cycle_(cycle),
            kind_(kind)
        { }

        bool inCycle() const {
            return cycle_;
        }
        const MoveOperand &from() const {
            return from_;
        }
        const MoveOperand &to() const {
            return to_;
        }
        Kind kind() const {
            return kind_;
        }
    };

  private:
    struct PendingMove
      : public Move,
        public TempObject,
        public InlineListNode<PendingMove>
    {
        PendingMove()
        { }
        PendingMove(const MoveOperand &from, const MoveOperand &to, Kind kind)
          : Move(from, to, kind, false)
        { }
        
        void setInCycle() {
            JS_ASSERT(!inCycle());
            cycle_ = true;
        }

    };

    typedef InlineList<MoveResolver::PendingMove>::iterator PendingMoveIterator;

  private:
    
    
    js::Vector<Move, 16, SystemAllocPolicy> orderedMoves_;
    bool hasCycles_;

    TempObjectPool<PendingMove> movePool_;

    InlineList<PendingMove> pending_;

    PendingMove *findBlockingMove(const PendingMove *last);

    
    void resetState();

  public:
    MoveResolver();

    
    
    
    
    
    
    
    bool addMove(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    bool resolve();

    size_t numMoves() const {
        return orderedMoves_.length();
    }
    const Move &getMove(size_t i) const {
        return orderedMoves_[i];
    }
    bool hasCycles() const {
        return hasCycles_;
    }
};

} 
} 

#endif 

