








































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
    static MDefinition *boxAt(MInstruction *at, MDefinition *operand);

  public:
    virtual bool adjustInputs(MInstruction *def);
};

class ArithPolicy : public BoxInputsPolicy
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
    ComparePolicy()
      : specialization_(MIRType_None)
    {
    }

    bool adjustInputs(MInstruction *def);
};


class TestPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};


class StringPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};


template <unsigned Op>
class IntPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};


template <unsigned Op>
class DoublePolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

template <unsigned Op>
class ObjectPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins);
    bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};



class SingleObjectPolicy : public ObjectPolicy<0>
{ };

template <unsigned Op>
class BoxPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins);
    bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};

template <unsigned Op>
class ArgumentsPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins);
    bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};


class SimplePolicy : public BoxInputsPolicy
{
    bool specialized_;

  public:
    SimplePolicy()
      : specialized_(true)
    { }

    bool adjustInputs(MInstruction *def);
    bool specialized() const {
        return specialized_;
    }
    void unspecialize() {
        specialized_ = false;
    }
};


template <class Lhs, class Rhs>
class MixPolicy
  : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def) {
        return Lhs::staticAdjustInputs(def) && Rhs::staticAdjustInputs(def);
    }
    virtual bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};

class CallSetElementPolicy : public SingleObjectPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};



class InstanceOfPolicy : public TypePolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};

class StoreTypedArrayPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};


class ClampPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
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

