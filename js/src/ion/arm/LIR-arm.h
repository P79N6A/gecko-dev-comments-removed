








































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

class LBoxDouble : public LInstructionHelper<2, 1, 1>
{
  public:
    LIR_HEADER(BoxDouble);

    LBoxDouble(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
};

class LUnbox : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(Unbox);

    MUnbox *mir() const {
        return mir_->toUnbox();
    }
    const LAllocation *type() {
        return getOperand(1);
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
    MUnbox *mir() const {
        return mir_->toUnbox();
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
          const LDefinition &temp1, const LDefinition &temp2)
    {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp1);
        setTemp(1, temp2);
    }
};

class LTableSwitch : public LInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(TableSwitch);

    LTableSwitch(const LAllocation &in, const LDefinition &inputCopy, MTableSwitch *ins)
    {
        setOperand(0, in);
        setTemp(0, inputCopy);
        setMir(ins);
    }

    MTableSwitch *mir() const {
        return mir_->toTableSwitch();
    }

    const LAllocation *index() {
        return getOperand(0);
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
};


class LGuardShape : public LInstructionHelper<0, 1, 1>
{
  public:
    LIR_HEADER(GuardShape);

    LGuardShape(const LAllocation &in, const LDefinition &temp) {
        setOperand(0, in);
        setTemp(0, temp);
    }
    const MGuardShape *mir() const {
        return mir_->toGuardShape();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
};

class LRecompileCheck : public LInstructionHelper<0, 0, 1>
{
  public:
    LIR_HEADER(RecompileCheck);

    LRecompileCheck(const LDefinition &temp) {
        setTemp(0, temp);
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
};

class LMulI : public LBinaryMath<0>
{
  public:
    LIR_HEADER(MulI);

    MMul *mir() {
        return mir_->toMul();
    }
};

} 
} 

#endif 

