





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
    explicit MoveOperand(const FloatRegister &reg) : kind_(FLOAT_REG), code_(reg.code())
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

  public:
    enum Type {
        GENERAL,
        INT32,
        FLOAT32,
        DOUBLE
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
        type_(type)
    { }

    bool isCycleBegin() const {
        return cycleBegin_;
    }
    bool isCycleEnd() const {
        return cycleEnd_;
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

        void setCycleBegin(Type endCycleType) {
            JS_ASSERT(!isCycleBegin() && !isCycleEnd());
            cycleBegin_ = true;
            endCycleType_ = endCycleType;
        }
        void setCycleEnd() {
            JS_ASSERT(!isCycleBegin() && !isCycleEnd());
            cycleEnd_ = true;
        }
    };

    typedef InlineList<MoveResolver::PendingMove>::iterator PendingMoveIterator;

  private:
    
    
    js::Vector<MoveOp, 16, SystemAllocPolicy> orderedMoves_;
    bool hasCycles_;

    TempObjectPool<PendingMove> movePool_;

    InlineList<PendingMove> pending_;

    PendingMove *findBlockingMove(const PendingMove *last);

    
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
    bool hasCycles() const {
        return hasCycles_;
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
