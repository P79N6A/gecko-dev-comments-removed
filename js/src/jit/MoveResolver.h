





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
    explicit MoveOperand(const Register &reg) : kind_(REG), code_(reg.code())
    { }
    explicit MoveOperand(const FloatRegister &reg) : kind_(FLOAT_REG), code_(reg.code())
    { }
    MoveOperand(const Register &reg, int32_t disp, Kind kind = MEMORY)
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
    bool cycle_;

  public:
    enum Kind {
        GENERAL,
        FLOAT32,
        DOUBLE
    };

  protected:
    Kind kind_;

  public:
    MoveOp()
    { }
    MoveOp(const MoveOperand &from, const MoveOperand &to, Kind kind)
      : from_(from),
        to_(to),
        cycle_(false),
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
        PendingMove(const MoveOperand &from, const MoveOperand &to, Kind kind)
          : MoveOp(from, to, kind)
        { }

        void setInCycle() {
            JS_ASSERT(!inCycle());
            cycle_ = true;
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

    
    
    
    
    
    
    
    bool addMove(const MoveOperand &from, const MoveOperand &to, MoveOp::Kind kind);
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
