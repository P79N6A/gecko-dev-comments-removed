





#ifndef jit_MoveResolver_h
#define jit_MoveResolver_h

#include "jit/InlineList.h"
#include "jit/IonAllocPolicy.h"
#include "jit/Registers.h"

namespace js {
namespace jit {



class MoveOperand
{
  public:
    enum Kind {
        
        REG,
        
        FLOAT_REG,
        
        MEMORY,
        
        EFFECTIVE_ADDRESS
    };

  private:
    Kind kind_;
    uint32_t code_;
    int32_t disp_;

  public:
    MoveOperand()
    { }
    explicit MoveOperand(Register reg) : kind_(REG), code_(reg.code())
    { }
    explicit MoveOperand(FloatRegister reg) : kind_(FLOAT_REG), code_(reg.code())
    { }
    MoveOperand(Register reg, int32_t disp, Kind kind = MEMORY)
        : kind_(kind),
        code_(reg.code()),
        disp_(disp)
    {
        JS_ASSERT(isMemoryOrEffectiveAddress());

        
        if (disp == 0 && kind_ == EFFECTIVE_ADDRESS)
            kind_ = REG;
    }
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
    bool isMemory() const {
        return kind_ == MEMORY;
    }
    bool isEffectiveAddress() const {
        return kind_ == EFFECTIVE_ADDRESS;
    }
    bool isMemoryOrEffectiveAddress() const {
        return isMemory() || isEffectiveAddress();
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
        JS_ASSERT(isMemoryOrEffectiveAddress());
        return Register::FromCode(code_);
    }
    int32_t disp() const {
        JS_ASSERT(isMemoryOrEffectiveAddress());
        return disp_;
    }

    bool aliases(MoveOperand other) const {

        
        
        

        JS_ASSERT_IF(isMemoryOrEffectiveAddress() && other.isGeneralReg(),
                     base() != other.reg());
        JS_ASSERT_IF(other.isMemoryOrEffectiveAddress() && isGeneralReg(),
                     other.base() != reg());

        if (kind_ != other.kind_)
            return false;
        if (kind_ == FLOAT_REG)
            return floatReg().aliases(other.floatReg());
        if (code_ != other.code_)
            return false;
        if (isMemoryOrEffectiveAddress())
            return disp_ == other.disp_;
        return true;
    }

    bool operator ==(const MoveOperand &other) const {
        if (kind_ != other.kind_)
            return false;
        if (code_ != other.code_)
            return false;
        if (isMemoryOrEffectiveAddress())
            return disp_ == other.disp_;
        return true;
    }
    bool operator !=(const MoveOperand &other) const {
        return !operator==(other);
    }
};


class MoveOp
{
  protected:
    MoveOperand from_;
    MoveOperand to_;
    bool cycleBegin_;
    bool cycleEnd_;
    int cycleBeginSlot_;
    int cycleEndSlot_;
  public:
    enum Type {
        GENERAL,
        INT32,
        FLOAT32,
        DOUBLE,
        INT32X4,
        FLOAT32X4
    };

  protected:
    Type type_;

    
    
    
    
    
    
    Type endCycleType_;

  public:
    MoveOp()
    { }
    MoveOp(const MoveOperand &from, const MoveOperand &to, Type type)
      : from_(from),
        to_(to),
        cycleBegin_(false),
        cycleEnd_(false),
        cycleBeginSlot_(-1),
        cycleEndSlot_(-1),
        type_(type)
    { }

    bool isCycleBegin() const {
        return cycleBegin_;
    }
    bool isCycleEnd() const {
        return cycleEnd_;
    }
    uint32_t cycleBeginSlot() const {
        MOZ_ASSERT(cycleBeginSlot_ != -1);
        return cycleBeginSlot_;
    }
    uint32_t cycleEndSlot() const {
        MOZ_ASSERT(cycleEndSlot_ != -1);
        return cycleEndSlot_;
    }
    const MoveOperand &from() const {
        return from_;
    }
    const MoveOperand &to() const {
        return to_;
    }
    Type type() const {
        return type_;
    }
    Type endCycleType() const {
        JS_ASSERT(isCycleBegin());
        return endCycleType_;
    }
};

class MoveResolver
{
  private:
    struct PendingMove
      : public MoveOp,
        public TempObject,
        public InlineListNode<PendingMove>
    {
        PendingMove()
        { }
        PendingMove(const MoveOperand &from, const MoveOperand &to, Type type)
          : MoveOp(from, to, type)
        { }

        void setCycleBegin(Type endCycleType, int cycleSlot) {
            JS_ASSERT(!cycleBegin_);
            cycleBegin_ = true;
            cycleBeginSlot_ = cycleSlot;
            endCycleType_ = endCycleType;
        }
        void setCycleEnd(int cycleSlot) {
            JS_ASSERT(!cycleEnd_);
            cycleEnd_ = true;
            cycleEndSlot_ = cycleSlot;
        }
    };

    typedef InlineList<MoveResolver::PendingMove>::iterator PendingMoveIterator;

  private:
    
    
    js::Vector<MoveOp, 16, SystemAllocPolicy> orderedMoves_;
    int numCycles_;
    int curCycles_;
    TempObjectPool<PendingMove> movePool_;

    InlineList<PendingMove> pending_;

    PendingMove *findBlockingMove(const PendingMove *last);
    PendingMove *findCycledMove(PendingMoveIterator *stack, PendingMoveIterator end, const PendingMove *first);
    
    void resetState();

  public:
    MoveResolver();

    
    
    
    
    
    
    
    bool addMove(const MoveOperand &from, const MoveOperand &to, MoveOp::Type type);
    bool resolve();

    size_t numMoves() const {
        return orderedMoves_.length();
    }
    const MoveOp &getMove(size_t i) const {
        return orderedMoves_[i];
    }
    uint32_t numCycles() const {
        return numCycles_;
    }
    void clearTempObjectPool() {
        movePool_.clear();
    }
    void setAllocator(TempAllocator &alloc) {
        movePool_.setAllocator(alloc);
    }
};

} 
} 

#endif 
