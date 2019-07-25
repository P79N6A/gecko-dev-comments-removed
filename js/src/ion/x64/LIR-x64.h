








































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



class LUnboxBoolean : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(UnboxBoolean);

    LUnboxBoolean(const LAllocation &input, const LDefinition &temp) {
        setOperand(0, input);
        setTemp(0, temp);
    }
};



class LUnboxInteger : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(UnboxInteger);

    LUnboxInteger(const LAllocation &input) {
        setOperand(0, input);
    }

    static const size_t Input = 0;

    const LDefinition *output() {
        return getDef(0);
    }
};



class LUnboxDouble : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(UnboxDouble);

    LUnboxDouble(const LAllocation &input) {
        setOperand(0, input);
    }

    static const size_t Input = 0;

    const LDefinition *output() {
        return getDef(0);
    }
};



class LUnboxObject : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(UnboxObject);

    LUnboxObject(const LAllocation &input) {
        setOperand(0, input);
    }
};



class LUnboxString : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(UnboxString);

    LUnboxString(const LAllocation &input, const LDefinition &temp) {
        setOperand(0, input);
        setTemp(0, temp);
    }
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

