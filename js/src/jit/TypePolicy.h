





#ifndef jit_TypePolicy_h
#define jit_TypePolicy_h

#include "jit/IonTypes.h"
#include "jit/JitAllocPolicy.h"

namespace js {
namespace jit {

class MInstruction;
class MDefinition;

extern MDefinition *
AlwaysBoxAt(TempAllocator &alloc, MInstruction *at, MDefinition *operand);



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

class NoTypePolicy
{
  public:
    struct Data
    {
        static TypePolicy *thisTypePolicy() {
            return nullptr;
        }
    };
};

class BoxInputsPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};

class ArithPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};

class AllDoublePolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator &alloc, MInstruction *def);
};

class BitwisePolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};

class ComparePolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};


class TestPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class TypeBarrierPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class CallPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};


class PowPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};


template <unsigned Op>
class StringPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToStringPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class IntPolicy MOZ_FINAL : private TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToInt32Policy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class DoublePolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class Float32Policy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned Op>
class FloatingPointPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};

template <unsigned Op>
class NoFloatPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned FirstOp>
class NoFloatPolicyAfter MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};


class ToDoublePolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


class ToInt32Policy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};


class ToStringPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};

template <unsigned Op>
class ObjectPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};



typedef ObjectPolicy<0> SingleObjectPolicy;



template <unsigned Op>
class SimdScalarPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *def);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, def);
    }
};

class SimdAllPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

template <unsigned Op>
class SimdPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class SimdSelectPolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class SimdSwizzlePolicy MOZ_FINAL : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};



template <unsigned Op>
class SimdSameAsReturnedTypePolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};

template <unsigned Op>
class BoxPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};


template <unsigned Op, MIRType Type>
class BoxExceptPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins);
    bool adjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Lhs, class Rhs>
class MixPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return Lhs::staticAdjustInputs(alloc, ins) && Rhs::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Policy1, class Policy2, class Policy3>
class Mix3Policy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return Policy1::staticAdjustInputs(alloc, ins) &&
               Policy2::staticAdjustInputs(alloc, ins) &&
               Policy3::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Policy1, class Policy2, class Policy3, class Policy4>
class Mix4Policy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator &alloc, MInstruction *ins) {
        return Policy1::staticAdjustInputs(alloc, ins) &&
               Policy2::staticAdjustInputs(alloc, ins) &&
               Policy3::staticAdjustInputs(alloc, ins) &&
               Policy4::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE {
        return staticAdjustInputs(alloc, ins);
    }
};

class CallSetElementPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};



class InstanceOfPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};

class StoreTypedArrayHolePolicy;
class StoreTypedArrayElementStaticPolicy;

class StoreTypedArrayPolicy : public TypePolicy
{
  private:
    static bool adjustValueInput(TempAllocator &alloc, MInstruction *ins, int arrayType, MDefinition *value, int valueOperand);

    friend class StoreTypedArrayHolePolicy;
    friend class StoreTypedArrayElementStaticPolicy;

  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class StoreTypedArrayHolePolicy MOZ_FINAL : public StoreTypedArrayPolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class StoreTypedArrayElementStaticPolicy MOZ_FINAL : public StoreTypedArrayPolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class StoreUnboxedObjectOrNullPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *def) MOZ_OVERRIDE;
};


class ClampPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
};

class FilterTypeSetPolicy MOZ_FINAL : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator &alloc, MInstruction *ins) MOZ_OVERRIDE;
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
