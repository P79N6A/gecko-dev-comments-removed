





#ifndef jit_x64_LIR_x64_h
#define jit_x64_LIR_x64_h

namespace js {
namespace jit {


class LBox : public LInstructionHelper<1, 1, 0>
{
    MIRType type_;

  public:
    LIR_HEADER(Box)

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
    LUnboxBase(const LAllocation &input) {
        setOperand(0, input);
    }

    static const size_t Input = 0;

    MUnbox *mir() const {
        return mir_->toUnbox();
    }
};

class LUnbox : public LUnboxBase {
  public:
    LIR_HEADER(Unbox)

    LUnbox(const LAllocation &input)
      : LUnboxBase(input)
    { }
};

class LUnboxFloatingPoint : public LUnboxBase {
    MIRType type_;

  public:
    LIR_HEADER(UnboxFloatingPoint)

    LUnboxFloatingPoint(const LAllocation &input, MIRType type)
      : LUnboxBase(input),
        type_(type)
    { }

    MIRType type() const {
        return type_;
    }
};


class LAsmJSUInt32ToDouble : public LInstructionHelper<1, 1, 0>
{
  public:
    LIR_HEADER(AsmJSUInt32ToDouble)

    LAsmJSUInt32ToDouble(const LAllocation &input) {
        setOperand(0, input);
    }
};

class LAsmJSLoadFuncPtr : public LInstructionHelper<1, 1, 1>
{
  public:
    LIR_HEADER(AsmJSLoadFuncPtr);
    LAsmJSLoadFuncPtr(const LAllocation &index, const LDefinition &temp) {
        setOperand(0, index);
        setTemp(0, temp);
    }
    MAsmJSLoadFuncPtr *mir() const {
        return mir_->toAsmJSLoadFuncPtr();
    }
    const LAllocation *index() {
        return getOperand(0);
    }
    const LDefinition *temp() {
        return getTemp(0);
    }
};

} 
} 

#endif 
