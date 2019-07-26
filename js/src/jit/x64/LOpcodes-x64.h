





#ifndef jit_x64_LOpcodes_x64_h
#define jit_x64_LOpcodes_x64_h

#define LIR_CPU_OPCODE_LIST(_)      \
    _(Box)                          \
    _(Unbox)                        \
    _(UnboxDouble)                  \
    _(DivI)                         \
    _(DivPowTwoI)                   \
    _(DivSelfI)                     \
    _(ModI)                         \
    _(ModPowTwoI)                   \
    _(PowHalfD)                     \
    _(UInt32ToDouble)               \
    _(AsmJSLoadFuncPtr)             \
    _(UDivOrMod)

#endif 
