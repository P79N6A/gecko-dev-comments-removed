





#ifndef jit_TypePolicy_h
#define jit_TypePolicy_h

#include "jit/IonTypes.h"

namespace js {
namespace jit {

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
    static MDefinition *alwaysBoxAt(MInstruction *at, MDefinition *operand);
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

    MIRType specialization() const {
        return specialization_;
    }
};

class ComparePolicy : public BoxInputsPolicy
{
    bool adjustInputs(MInstruction *def);
};


class TestPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

class TypeBarrierPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(MInstruction *def);
};


class PowPolicy : public BoxInputsPolicy
{
    MIRType specialization_;

  public:
    PowPolicy(MIRType specialization)
      : specialization_(specialization)
    { }

    bool adjustInputs(MInstruction *ins);
};


template <unsigned Op>
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
class Float32Policy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};



template <unsigned Op>
class FloatingPointPolicy : public TypePolicy
{
    MIRType policyType_;

  public:
    bool adjustInputs(MInstruction *def) {
        if (policyType_ == MIRType_Double)
            return DoublePolicy<Op>::staticAdjustInputs(def);
        return Float32Policy<Op>::staticAdjustInputs(def);
    }
    void setPolicyType(MIRType type) {
        policyType_ = type;
    }
};

template <unsigned Op>
class NoFloatPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *def);
    bool adjustInputs(MInstruction *def) {
        return staticAdjustInputs(def);
    }
};


class ToDoublePolicy : public BoxInputsPolicy
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


template <class Lhs, class Rhs>
class MixPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins) {
        return Lhs::staticAdjustInputs(ins) && Rhs::staticAdjustInputs(ins);
    }
    virtual bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
    }
};


template <class Policy1, class Policy2, class Policy3>
class Mix3Policy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(MInstruction *ins) {
        return Policy1::staticAdjustInputs(ins) && Policy2::staticAdjustInputs(ins) &&
               Policy3::staticAdjustInputs(ins);
    }
    virtual bool adjustInputs(MInstruction *ins) {
        return staticAdjustInputs(ins);
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
  protected:
    bool adjustValueInput(MInstruction *ins, int arrayType, MDefinition *value, int valueOperand);

  public:
    bool adjustInputs(MInstruction *ins);
};

class StoreTypedArrayHolePolicy : public StoreTypedArrayPolicy
{
  public:
    bool adjustInputs(MInstruction *ins);
};

class StoreTypedArrayElementStaticPolicy : public StoreTypedArrayPolicy
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
    if (type == MIRType_Undefined || IsFloatingPointType(type))
        return true;
    return false;
}


} 
} 

#endif 
