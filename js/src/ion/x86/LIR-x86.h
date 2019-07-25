








































#ifndef jsion_lir_x86_h__
#define jsion_lir_x86_h__

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

class LBoxDouble : public LInstructionHelper<2, 1, 1>
{
  public:
    LIR_HEADER(BoxDouble);

    LBoxDouble(const LAllocation &in) {
        setOperand(0, in);
        setTemp(0, LDefinition(LDefinition::DOUBLE));
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

class LUnboxDouble : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(UnboxDouble);

    LUnboxDouble() {
        setTemp(0, LDefinition(LDefinition::DOUBLE));
    }
};

class LUnboxDoubleSSE41 : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(UnboxDoubleSSE41);
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
};

} 
} 

#endif 

