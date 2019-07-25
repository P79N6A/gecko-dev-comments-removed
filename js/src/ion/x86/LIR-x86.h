








































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

class LBoxToDouble : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(BoxToDouble);
};

} 
} 

#endif 

