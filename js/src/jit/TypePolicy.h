





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

struct TypeSpecializationData
{
  protected:
    
    
    
    MIRType specialization_;

    MIRType thisTypeSpecialization() {
        return specialization_;
    }

  public:
    MIRType specialization() const {
        return specialization_;
    }
};

#define EMPTY_DATA_                                     \
    struct Data                                         \
    {                                                   \
        static TypePolicy *thisTypePolicy();            \
    }

#define INHERIT_DATA_(DATA_TYPE)                        \
    struct Data : public DATA_TYPE                      \
    {                                                   \
        static TypePolicy *thisTypePolicy();            \
    }

#define SPECIALIZATION_DATA_ INHERIT_DATA_(TypeSpecializationData)

class BoxInputsPolicy : public TypePolicy
{
  protected:
    static MDefinition *boxAt(TempAllocator &alloc, MInstruction *at, MDefinition *operand);

  public:
    EMPTY_DATA_;
    static MDefinition *alwaysBoxAt(TempAllocator &alloc, MInstruction *at, MDefinition *operand);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class ArithPolicy : public BoxInputsPolicy
{
  public:
    SPECIALIZATION_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class BitwisePolicy : public BoxInputsPolicy
{
  public:
    SPECIALIZATION_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class ComparePolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};


class TestPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class TypeBarrierPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class CallPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};


class PowPolicy : public BoxInputsPolicy
{
  public:
    SPECIALIZATION_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};


template <unsigned Op>
class StringPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToStringPolicy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class IntPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToInt32Policy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class DoublePolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class Float32Policy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned Op>
class FloatingPointPolicy : public TypePolicy
{

  public:
    struct PolicyTypeData
    {
        MIRType policyType_;

        void setPolicyType(MIRType type) {
            policyType_ = type;
        }

      protected:
        MIRType &thisTypeSpecialization() {
            return policyType_;
        }
    };

    INHERIT_DATA_(PolicyTypeData);

    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

template <unsigned Op>
class NoFloatPolicy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToDoublePolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToInt32Policy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};


class ToStringPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    bool adjustInputs(TempAllocator &alloc, MInstruction *def) {
        return staticAdjustInputs(alloc, def);
    }
};

template <unsigned Op>
class ObjectPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};



typedef ObjectPolicy<0> SingleObjectPolicy;

template <unsigned Op>
class BoxPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <unsigned Op, MIRType Type>
class BoxExceptPolicy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Lhs, class Rhs>
class MixPolicy : public TypePolicy
{
  public:
    EMPTY_DATA_;
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
    EMPTY_DATA_;
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
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};



class InstanceOfPolicy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class StoreTypedArrayPolicy : public BoxInputsPolicy
{
  protected:
    bool adjustValueInput(TempAllocator &alloc, MInstruction *ins, int arrayType, MDefinition *value, int valueOperand);

  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class StoreTypedArrayHolePolicy : public StoreTypedArrayPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class StoreTypedArrayElementStaticPolicy : public StoreTypedArrayPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};


class ClampPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

class FilterTypeSetPolicy : public BoxInputsPolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins);
};

static inline bool
CoercesToDouble(MIRType type)
{
    if (type == MIRType_Undefined || IsFloatingPointType(type))
        return true;
    return false;
}

#undef SPECIALIZATION_DATA_
#undef INHERIT_DATA_
#undef EMPTY_DATA_

} 
} 

#endif 
