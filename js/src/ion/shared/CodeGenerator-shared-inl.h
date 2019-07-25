








































#ifndef jsion_codegen_inl_h__
#define jsion_codegen_inl_h__

namespace js {
namespace ion {

static inline Register
ToRegister(const LAllocation &a)
{
    JS_ASSERT(a.isGeneralReg());
    return a.toGeneralReg()->reg();
}

static inline Register
ToRegister(const LAllocation *a)
{
    return ToRegister(*a);
}

static inline Register
ToRegister(const LDefinition *def)
{
    return ToRegister(*def->output());
}

static inline FloatRegister
ToFloatRegister(const LAllocation &a)
{
    JS_ASSERT(a.isFloatReg());
    return a.toFloatReg()->reg();
}

static inline FloatRegister
ToFloatRegister(const LAllocation *a)
{
    return ToFloatRegister(*a);
}

static inline FloatRegister
ToFloatRegister(const LDefinition *def)
{
    return ToFloatRegister(*def->output());
}

} 
} 

#endif 

