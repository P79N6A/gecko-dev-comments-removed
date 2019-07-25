








































#ifndef jsion_type_policy_h__
#define jsion_type_policy_h__

#include "TypeOracle.h"

namespace js {
namespace ion {

class MInstruction;
class MDefinition;

class TypeAnalysis
{
  public:
    virtual void addPreferredType(MDefinition *def, MIRType type) = 0;
    inline void preferType(MDefinition *def, MIRType type);
};



class TypePolicy
{
  public:
    
    
    
    
    
    
    
    virtual bool respecialize(MInstruction *def) = 0;

    
    
    virtual void specializeInputs(MInstruction *ins, TypeAnalysis *analysis) = 0;

    
    
    
    
    
    
    virtual bool adjustInputs(MInstruction *def) = 0;
};

class BoxInputsPolicy : public TypePolicy
{
  protected:
    MDefinition *boxAt(MInstruction *at, MDefinition *operand);

  public:
    virtual bool respecialize(MInstruction *def);
    virtual void specializeInputs(MInstruction *ins, TypeAnalysis *analyzer);
    virtual bool adjustInputs(MInstruction *def);
};

class BinaryArithPolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool respecialize(MInstruction *def);
    void specializeInputs(MInstruction *ins, TypeAnalysis *analyzer);
    bool adjustInputs(MInstruction *def);
};

class BitwisePolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool respecialize(MInstruction *def);
    void specializeInputs(MInstruction *ins, TypeAnalysis *analyzer);
    bool adjustInputs(MInstruction *def);
};

class TableSwitchPolicy : public BoxInputsPolicy
{
  public:
    bool respecialize(MInstruction *def);
    void specializeInputs(MInstruction *ins, TypeAnalysis *analyzer);
    bool adjustInputs(MInstruction *def);
};

class ComparePolicy : public BoxInputsPolicy
{
  protected:
    MIRType specialization_;

  public:
    bool respecialize(MInstruction *def);
    void specializeInputs(MInstruction *ins, TypeAnalysis *analyzer);
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

