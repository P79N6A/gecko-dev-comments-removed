








































#ifndef jsion_lir_inl_h__
#define jsion_lir_inl_h__

namespace js {
namespace ion {

#define LIROP(name)                                                         \
    L##name *LInstruction::to##name()                                       \
    {                                                                       \
        JS_ASSERT(is##name());                                              \
        return static_cast<L##name *>(this);                                \
    }
    LIR_OPCODE_LIST(LIROP)
#undef LIROP

#define LALLOC_CAST(type)                                                   \
    L##type *LAllocation::to##type() {                                      \
        JS_ASSERT(is##type());                                              \
        return static_cast<L##type *>(this);                                \
    }
#define LALLOC_CONST_CAST(type)                                             \
    const L##type *LAllocation::to##type() const {                          \
        JS_ASSERT(is##type());                                              \
        return static_cast<const L##type *>(this);                          \
    }

LALLOC_CAST(Use)
LALLOC_CONST_CAST(Use)
LALLOC_CONST_CAST(GeneralReg)
LALLOC_CONST_CAST(FloatReg)
LALLOC_CONST_CAST(StackSlot)
LALLOC_CONST_CAST(Argument)
LALLOC_CONST_CAST(ConstantIndex)

#undef LALLOC_CAST

} 
} 

#endif 

