








































#ifndef jsion_lir_arm_h__
#define jsion_lir_arm_h__

#include "ion/TypeOracle.h"

namespace js {
namespace ion {

class LBox : public LInstructionHelper<2, 1, 0>
{
    MIRType type_;

  public:
    LIR_HEADER(Box);

    LBox(const LAllocation &in_payload, MIRType type)
      : type_(type)
    {
        setOperand(0, in_payload);
    }

    MIRType type() const {
        return type_;
    }
};

class LBoxDouble : public LInstructionHelper<2, 1, 0>
{
  public:
    LIR_HEADER(BoxDouble);

    LBoxDouble(const LAllocation &in) {
        setOperand(0, in);
    }
};

class LUnbox : public LInstructionHelper<1, 2, 0>
{
    MIRType type_;

  public:
    LIR_HEADER(Unbox);

    LUnbox(MIRType type)
      : type_(type)
    { }

    MIRType type() const {
        return type_;
    }
};

class LUnboxDouble : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(UnboxDouble);

    static const size_t Input = 0;

    const LDefinition *output() {
        return getDef(0);
    }
};


class LDouble : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Double);

    LDouble(const LConstantIndex &cindex) {
        setOperand(0, cindex);
    }
};

class LDivI : public LBinaryMath<2>
{
  public:
    LIR_HEADER(DivI);

    LDivI(const LAllocation &lhs, const LAllocation &rhs,
          const LDefinition &temp1, const LDefinition &temp2) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }

    const LDefinition *remainder() {
        return getTemp(0);
    }
};

} 
} 

#endif 

