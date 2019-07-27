





#ifndef jit_arm64_LOpcodes_arm64_h
#define jit_arm64_LOpcodes_arm64_h

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxFloatingPoint)       \
    _(Box)                      \
    _(DivI)                     \
    _(SoftDivI)                 \
    _(DivPowTwoI)               \
    _(ModI)                     \
    _(SoftModI)                 \
    _(ModPowTwoI)               \
    _(ModMaskI)                 \
    _(PowHalfD)                 \
    _(AsmJSUInt32ToDouble)      \
    _(AsmJSUInt32ToFloat32)     \
    _(UDiv)                     \
    _(UMod)                     \
    _(SoftUDivOrMod)            \
    _(AsmJSLoadFuncPtr)

#endif 
