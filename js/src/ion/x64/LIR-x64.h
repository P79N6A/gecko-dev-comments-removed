






#ifndef jsion_lir_x64_h__
#define jsion_lir_x64_h__

#include "ion/TypeOracle.h"

namespace js {
namespace ion {


class LBox : public LInstructionHelper<1, 1, 0>
{
    MIRType type_;

  public:
    LIR_HEADER(Box);

    LBox(MIRType type, const LAllocation &payload)
      : type_(type)
    {
        setOperand(0, payload);
    }

    MIRType type() const {
        return type_;
    }
};



class LUnboxBase : public LInstructionHelper<1, 1, 0>
{
  public:
    LUnboxBase(const LAllocation &input)
    {
        setOperand(0, input);
    }

    static const size_t Input = 0;

    const LDefinition *output() {
        return getDef(0);
    }
    MUnbox *mir() const {
        return mir_->toUnbox();
    }
};

class LUnbox : public LUnboxBase {
  public:
    LIR_HEADER(Unbox);

    LUnbox(const LAllocation &input)
      : LUnboxBase(input)
    { }
};

class LUnboxDouble : public LUnboxBase {
  public:
    LIR_HEADER(UnboxDouble);

    LUnboxDouble(const LAllocation &input)
      : LUnboxBase(input)
    { }
};


class LDouble : public LInstructionHelper<1, 0, 0>
{
    double d_;

  public:
    LIR_HEADER(Double);

    LDouble(double d) : d_(d)
    { }

    double getDouble() const {
        return d_;
    }
    const LDefinition *output() {
        return getDef(0);
    }
};

} 
} 

#endif 

