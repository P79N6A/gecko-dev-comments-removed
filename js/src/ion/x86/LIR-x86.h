








































#ifndef jsion_lir_x86_h__
#define jsion_lir_x86_h__

#include "ion/TypeOracle.h"

namespace js {
namespace ion {

class LBox : public LInstructionHelper<2, 1, 0>
{
  public:
    LIR_HEADER(Box);

    LBox(const LAllocation &in_payload, const LDefinition &out_type, const LDefinition &out_payload)
    {
        setOperand(0, in_payload);
        setDef(0, out_type);
        setDef(1, out_payload);
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
};

class LBoxToDouble : public LInstructionHelper<1, 2, 1>
{
  public:
    LIR_HEADER(BoxToDouble);
};

} 
} 

#endif 

