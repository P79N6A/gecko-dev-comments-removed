








































#ifndef jsion_type_policy_h__
#define jsion_type_policy_h__

#include "TypeOracle.h"

namespace js {
namespace ion {

class MInstruction;
class MDefinition;



class TypePolicy
{
  public:
    
    
    
    
    
    
    virtual bool adjustInputs(MInstruction *def) = 0;
};

class BoxInputsPolicy : public TypePolicy
{
  protected:
    MDefinition *boxAt(MInstruction *at, MDefinition *operand);

  public:
    virtual bool adjustInputs(MInstruction *def);
};

class BinaryArithPolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool adjustInputs(MInstruction *def);
};

class BinaryStringPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class BitwisePolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool adjustInputs(MInstruction *def);
};

class TableSwitchPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class ComparePolicy : public BoxInputsPolicy
{
  protected:
    MIRType specialization_;

  public:
    bool adjustInputs(MInstruction *def);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};



class ObjectPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};


class StringPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

static inline bool
CoercesToDouble(MIRType type)
{
    if (type == MIRType_Undefined || type == MIRType_Double)
        return true;
    return false;
}


} 
} 

#endif 

