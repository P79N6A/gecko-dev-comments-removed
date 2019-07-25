








































#ifndef jsion_lir_inl_h__
#define jsion_lir_inl_h__

#define LIROP(name)                                                         \
    L##name *LInstruction::to##name()                                       \
    {                                                                       \
        JS_ASSERT(is##name());                                              \
        return static_cast<L##name *>(this);                                \
    }
# include "LOpcodes.tbl"
#undef LIROP

} 
} 

#endif 

