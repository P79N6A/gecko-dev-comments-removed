








































#ifndef jsion_lir_common_h__
#define jsion_lir_common_h__



namespace js {
namespace ion {


class LMove : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(Move);

    LMove(const LAllocation &from, const LAllocation &to) {
        setOperand(0, from);
        setDef(0, LDefinition(LDefinition::POINTER, to));
    }
};


class LInteger : public LInstructionHelper<1, 0, 0>
{
    int32 i32_;

  public:
    LIR_HEADER(Integer);

    LInteger(int32 i32) : i32_(i32)
    { }
};


class LPointer : public LInstructionHelper<1, 0, 0>
{
    void *ptr_;

  public:
    LIR_HEADER(Pointer);

    LPointer(void *ptr) : ptr_(ptr)
    { }
};


class LDouble : public LInstructionHelper<1, 0, 0>
{
    double d_;

  public:
    LIR_HEADER(Double);

    LDouble(double d) : d_(d)
    { }
};



class LParameter : public LInstructionHelper<BOX_PIECES, 0, 0>
{
  public:
    LIR_HEADER(Parameter);
};


class LGoto : public LInstructionHelper<0, 0, 0>
{
    MBasicBlock *block_;

  public:
    LIR_HEADER(Goto);

    LGoto(MBasicBlock *block)
      : block_(block)
    { }
};


class LTestIAndBranch : public LInstructionHelper<0, 1, 0>
{
    MBasicBlock *ifTrue;
    MBasicBlock *ifFalse;

  public:
    LIR_HEADER(TestIAndBranch);

    LTestIAndBranch(const LAllocation &in, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue(ifTrue),
        ifFalse(ifFalse)
    {
        setOperand(0, in);
    }
};


class LTestDAndBranch : public LInstructionHelper<0, 1, 1>
{
    MBasicBlock *ifTrue;
    MBasicBlock *ifFalse;

  public:
    LIR_HEADER(TestDAndBranch);

    LTestDAndBranch(const LAllocation &in, const LDefinition &temp,
                    MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue(ifTrue),
        ifFalse(ifFalse)
    {
        setOperand(0, in);
        setTemp(0, temp);
    }
};


class LTestVAndBranch : public LInstructionHelper<0, BOX_PIECES, 0>
{
    MBasicBlock *ifTrue;
    MBasicBlock *ifFalse;

  public:
    LIR_HEADER(TestVAndBranch);

    LTestVAndBranch(MBasicBlock *ifTrue, MBasicBlock *ifFalse)
      : ifTrue(ifTrue),
        ifFalse(ifFalse)
    { }
};



class LBitOp : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
    LIR_HEADER(BitOp);

    LBitOp(JSOp op, const LAllocation &left, const LAllocation &right)
      : op_(op)
    {
        setOperand(0, left);
        setOperand(1, right);
    }
};



class LReturn : public LInstructionHelper<0, BOX_PIECES, 0>
{
  public:
    LIR_HEADER(Return);
};


class LAddI : public LInstructionHelper<1, 2, 0>
{
  public:
    LIR_HEADER(AddI);
};

class MPhi;





class LPhi : public LInstruction
{
    uint32 numInputs_;
    LAllocation *inputs_;
    LDefinition def_;

    bool init(MIRGenerator *gen);

    LPhi(uint32 numInputs)
      : numInputs_(numInputs)
    { }

  public:
    LIR_HEADER(Phi);

    static LPhi *New(MIRGenerator *gen, MPhi *phi);

    size_t numDefs() const {
        return 1;
    }
    LDefinition *getDef(size_t index) {
        JS_ASSERT(index == 0);
        return &def_;
    }
    void setDef(size_t index, const LDefinition &def) {
        JS_ASSERT(index == 0);
        def_ = def;
    }
    size_t numOperands() const {
        return numInputs_;
    }
    LAllocation *getOperand(size_t index) {
        JS_ASSERT(index < numOperands());
        return &inputs_[index];
    }
    void setOperand(size_t index, const LAllocation &a) {
        JS_ASSERT(index < numOperands());
        inputs_[index] = a;
    }
    size_t numTemps() const {
        return 0;
    }
    LDefinition *getTemp(size_t index) {
        JS_NOT_REACHED("no temps");
        return NULL;
    }
    void setTemp(size_t index, const LDefinition &temp) {
        JS_NOT_REACHED("no temps");
    }

    virtual void printInfo(FILE *fp) {
        printOperands(fp);
    }
};

} 
} 

#endif 

