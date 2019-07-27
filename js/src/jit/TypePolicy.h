





#ifndef jit_TypePolicy_h
#define jit_TypePolicy_h

#include "jit/IonTypes.h"
#include "jit/JitAllocPolicy.h"

namespace js {
namespace jit {

class MInstruction;
class MDefinition;

extern MDefinition*
AlwaysBoxAt(TempAllocator& alloc, MInstruction* at, MDefinition* operand);



class TypePolicy
{
  public:
    
    
    
    
    
    
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) = 0;
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
        static TypePolicy* thisTypePolicy();            \
    }

#define INHERIT_DATA_(DATA_TYPE)                        \
    struct Data : public DATA_TYPE                      \
    {                                                   \
        static TypePolicy* thisTypePolicy();            \
    }

#define SPECIALIZATION_DATA_ INHERIT_DATA_(TypeSpecializationData)

class NoTypePolicy
{
  public:
    struct Data
    {
        static TypePolicy* thisTypePolicy() {
            return nullptr;
        }
    };
};

class BoxInputsPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};

class ArithPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};

class AllDoublePolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    bool adjustInputs(TempAllocator& alloc, MInstruction* def);
};

class BitwisePolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};

class ComparePolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};


class TestPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class TypeBarrierPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class CallPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};


class PowPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};


template <unsigned Op>
class StringPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToStringPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class IntPolicy final : private TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class ConvertToInt32Policy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class TruncateToInt32Policy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class DoublePolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


template <unsigned Op>
class Float32Policy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned Op>
class FloatingPointPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};

template <unsigned Op>
class NoFloatPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};



template <unsigned FirstOp>
class NoFloatPolicyAfter final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};


class ToDoublePolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


class ToInt32Policy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};


class ToStringPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};

template <unsigned Op>
class ObjectPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};



typedef ObjectPolicy<0> SingleObjectPolicy;



template <unsigned Op>
class SimdScalarPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* def);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override {
        return staticAdjustInputs(alloc, def);
    }
};

class SimdAllPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

template <unsigned Op>
class SimdPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class SimdSelectPolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class SimdShufflePolicy final : public TypePolicy
{
  public:
    SPECIALIZATION_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};



template <unsigned Op>
class SimdSameAsReturnedTypePolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};

template <unsigned Op>
class BoxPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins);
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};


template <unsigned Op, MIRType Type>
class BoxExceptPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins);
    bool adjustInputs(TempAllocator& alloc, MInstruction* ins) {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Lhs, class Rhs>
class MixPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins) {
        return Lhs::staticAdjustInputs(alloc, ins) && Rhs::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Policy1, class Policy2, class Policy3>
class Mix3Policy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins) {
        return Policy1::staticAdjustInputs(alloc, ins) &&
               Policy2::staticAdjustInputs(alloc, ins) &&
               Policy3::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};


template <class Policy1, class Policy2, class Policy3, class Policy4>
class Mix4Policy : public TypePolicy
{
  public:
    EMPTY_DATA_;
    static bool staticAdjustInputs(TempAllocator& alloc, MInstruction* ins) {
        return Policy1::staticAdjustInputs(alloc, ins) &&
               Policy2::staticAdjustInputs(alloc, ins) &&
               Policy3::staticAdjustInputs(alloc, ins) &&
               Policy4::staticAdjustInputs(alloc, ins);
    }
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override {
        return staticAdjustInputs(alloc, ins);
    }
};

class CallSetElementPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};



class InstanceOfPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};

class StoreTypedArrayHolePolicy;
class StoreTypedArrayElementStaticPolicy;

class StoreUnboxedScalarPolicy : public TypePolicy
{
  private:
    static bool adjustValueInput(TempAllocator& alloc, MInstruction* ins, Scalar::Type arrayType,
                                 MDefinition* value, int valueOperand);

    friend class StoreTypedArrayHolePolicy;
    friend class StoreTypedArrayElementStaticPolicy;

  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class StoreTypedArrayHolePolicy final : public StoreUnboxedScalarPolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class StoreTypedArrayElementStaticPolicy final : public StoreUnboxedScalarPolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class StoreUnboxedObjectOrNullPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* def) override;
};


class ClampPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
};

class FilterTypeSetPolicy final : public TypePolicy
{
  public:
    EMPTY_DATA_;
    virtual bool adjustInputs(TempAllocator& alloc, MInstruction* ins) override;
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
