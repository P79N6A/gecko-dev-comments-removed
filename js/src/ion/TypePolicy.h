








































#ifndef jsion_type_policy_h__
#define jsion_type_policy_h__

#include "TypeOracle.h"

namespace js {
namespace ion {

class MInstruction;
class MInstruction;



class TypePolicy
{
  public:
    
    
    
    
    
    
    
    
    virtual bool respecialize(MInstruction *def) = 0;

    
    
    
    
    
    
    virtual bool adjustInputs(MInstruction *def) = 0;

    
    
    
    virtual bool useSpecializedInput(MInstruction *def, size_t index, MInstruction *special) = 0;
};

class BoxInputPolicy : public TypePolicy
{
  public:
    virtual bool respecialize(MInstruction *def);
    virtual bool adjustInputs(MInstruction *def);
    virtual bool useSpecializedInput(MInstruction *def, size_t index, MInstruction *special);
};

class BinaryArithPolicy : public BoxInputPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool respecialize(MInstruction *def);
    bool adjustInputs(MInstruction *def);
    bool useSpecializedInput(MInstruction *def, size_t index, MInstruction *special);
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

