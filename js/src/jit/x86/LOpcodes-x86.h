





#ifndef jit_x86_LOpcodes_x86_h
#define jit_x86_LOpcodes_x86_h

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxFloatingPoint)       \
    _(Box)                      \
    _(BoxFloatingPoint)         \
    _(DivI)                     \
    _(DivPowTwoI)               \
    _(DivOrModConstantI)        \
    _(ModI)                     \
    _(ModPowTwoI)               \
    _(PowHalfD)                 \
    _(AsmJSUInt32ToDouble)      \
    _(AsmJSUInt32ToFloat32)     \
    _(AsmJSLoadFuncPtr)         \
    _(SimdValueInt32x4)         \
    _(SimdValueFloat32x4)       \
    _(UDivOrMod)

#endif 
