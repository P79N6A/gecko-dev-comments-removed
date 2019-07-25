








































#ifndef jsion_lir_x86_shared_h__
#define jsion_lir_x86_shared_h__

namespace js {
namespace ion {

class LDivI : public LBinaryMath<1>
{
  public:
    LIR_HEADER(DivI);

    LDivI(const LAllocation &lhs, const LAllocation &rhs, const LDefinition &temp) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp);
    }

    const LDefinition *remainder() {
        return getTemp(0);
    }
};

class LModI : public LBinaryMath<1>
{
  public:
    LIR_HEADER(ModI);

    LModI(const LAllocation &lhs, const LAllocation &rhs) {
        setOperand(0, lhs);
        setOperand(1, rhs);
    }

    const LDefinition *remainder() {
        return getDef(0);
    }
};


class LTableSwitch : public LInstructionHelper<0, 1, 2>
{
  public:
    LIR_HEADER(TableSwitch);

    LTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                 const LDefinition &jumpTablePointer, MTableSwitch *ins)
    {
        setOperand(0, in);
        setTemp(0, inputCopy);
        setTemp(1, jumpTablePointer);
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
    const LAllocation *tempPointer() {
        return getTemp(1)->output();
    }
};


class LGuardShape : public LInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(GuardShape);

    LGuardShape(const LAllocation &in) {
        setOperand(0, in);
    }
    const MGuardShape *mir() const {
        return mir_->toGuardShape();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
};

} 
} 

#endif 

