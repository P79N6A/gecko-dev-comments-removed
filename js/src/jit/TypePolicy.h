





#ifndef jit_TypePolicy_h
#define jit_TypePolicy_h

#include "jit/IonAllocPolicy.h"
#include "jit/IonTypes.h"

namespace js {
namespace jit {

class MInstruction;
class MDefinition;



class TypePolicy
{
  public:
    
    
    
    
    
    
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) = 0;
};

class BoxInputsPolicy : public TypePolicy
{
  protected:
    static MDefinition *boxAt(TempAllocator &alloc, MInstruction *at, MDefinition *operand);

  public:
    static MDefinition *alwaysBoxAt(TempAllocator &alloc, MInstruction *at, MDefinition *operand);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class ArithPolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class BitwisePolicy : public BoxInputsPolicy
{
  protected:
    
    
    
    
    MIRType specialization_;

  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);

    MIRType specialization() const {
        return specialization_;
    }
};

class ComparePolicy : public BoxInputsPolicy
{
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};


class TestPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class TypeBarrierPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};


class PowPolicy : public BoxInputsPolicy
{
    MIRType specialization_;

  public:
    explicit PowPolicy(MIRType specialization)
      : specialization_(specialization)
    { }

    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};


template <unsigned Op>
class StringPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToStringPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class IntPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToInt32Policy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class DoublePolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class Float32Policy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned Op>
class FloatingPointPolicy : public TypePolicy
{
    MIRType policyType_;

  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        if (policyType_ == MIRType_Double)
            return DoublePolicy<Op>::staticAdjustInputs(alloc, def);
        return Float32Policy<Op>::staticAdjustInputs(alloc, def);
    }
    void setPolicyType(MIRType type) {
        policyType_ = type;
    }
};

template <unsigned Op>
class NoFloatPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToDoublePolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToInt32Policy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToStringPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};

template <unsigned Op>
class ObjectPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};



class SingleObjectPolicy : public ObjectPolicy<0>
{ };

template <unsigned Op>
class BoxPolicy : public BoxInputsPolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <unsigned Op, MIRType Type>
class BoxExceptPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Lhs, class Rhs>
class MixPolicy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return Lhs::staticAdjustInputs(alloc, ins) && Rhs::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Policy1, class Policy2, class Policy3>
class Mix3Policy : public TypePolicy
{
  public:
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return Policy1::staticAdjustInputs(alloc, ins) &&
               Policy2::staticAdjustInputs(alloc, ins) &&
               Policy3::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};

class CallSetElementPolicy : public SingleObjectPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};



class InstanceOfPolicy : public TypePolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class StoreTypedArrayPolicy : public BoxInputsPolicy
{
  protected:
    bool adjustValueInput(TempAllocator &alloc, MInstruction *ins, int arrayType, MDefinition *value, int valueOperand);

  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class StoreTypedArrayHolePolicy : public StoreTypedArrayPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class StoreTypedArrayElementStaticPolicy : public StoreTypedArrayPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};


class ClampPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class FilterTypeSetPolicy : public BoxInputsPolicy
{
  public:
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
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
