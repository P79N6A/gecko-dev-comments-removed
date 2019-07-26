





#ifndef jit_x86_LOpcodes_x86_h
#define jit_x86_LOpcodes_x86_h

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxFloatingPoint)       \
    _(Box)                      \
    _(BoxFloatingPoint)         \
    _(DivI)                     \
    _(DivPowTwoI)               \
    _(DivSelfI)                 \
    _(ModI)                     \
    _(ModPowTwoI)               \
    _(ModSelfI)                 \
    _(PowHalfD)                 \
    _(AsmJSUInt32ToDouble)      \
    _(AsmJSUInt32ToFloat32)     \
    _(AsmJSLoadFuncPtr)         \
    _(UDivOrMod)

#endif 
