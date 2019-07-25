








































#ifndef jsion_lir_common_h__
#define jsion_lir_common_h__



namespace js {
namespace ion {


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
};

class LTest : public LInstruction
{
};



class LBitOp : public LInstructionHelper<1, 2, 0>
{
    JSOp op_;

  public:
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


} 
} 

#endif 

