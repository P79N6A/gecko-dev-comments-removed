





#ifndef jit_mips_LOpcodes_mips_h__
#define jit_mips_LOpcodes_mips_h__

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxFloatingPoint)       \
    _(Box)                      \
    _(BoxFloatingPoint)         \
    _(DivI)                     \
    _(DivPowTwoI)               \
    _(ModI)                     \
    _(ModPowTwoI)               \
    _(ModMaskI)                 \
    _(PowHalfD)                 \
    _(AsmJSUInt32ToDouble)      \
    _(AsmJSUInt32ToFloat32)     \
    _(UDiv)                     \
    _(UMod)                     \
    _(AsmJSLoadFuncPtr)

#endif 
