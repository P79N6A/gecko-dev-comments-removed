






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

#ifdef JS_NUNBOX32
static inline signed
OffsetToOtherHalfOfNunbox(LDefinition::Type type)
{
    JS_ASSERT(type == LDefinition::TYPE || type == LDefinition::PAYLOAD);
    signed offset = (type == LDefinition::TYPE)
                    ? PAYLOAD_INDEX - TYPE_INDEX
                    : TYPE_INDEX - PAYLOAD_INDEX;
    return offset;
}

static inline void
AssertTypesFormANunbox(LDefinition::Type type1, LDefinition::Type type2)
{
    JS_ASSERT((type1 == LDefinition::TYPE && type2 == LDefinition::PAYLOAD) ||
              (type2 == LDefinition::TYPE && type1 == LDefinition::PAYLOAD));
}

static inline unsigned
OffsetOfNunboxSlot(LDefinition::Type type)
{
    if (type == LDefinition::PAYLOAD)
        return NUNBOX32_PAYLOAD_OFFSET / STACK_SLOT_SIZE;
    return NUNBOX32_TYPE_OFFSET / STACK_SLOT_SIZE;
}



static inline unsigned
BaseOfNunboxSlot(LDefinition::Type type, unsigned slot)
{
    if (type == LDefinition::PAYLOAD)
        return slot + (NUNBOX32_PAYLOAD_OFFSET / STACK_SLOT_SIZE);
    return slot + (NUNBOX32_TYPE_OFFSET / STACK_SLOT_SIZE);
}
#endif

} 
} 

#endif 

