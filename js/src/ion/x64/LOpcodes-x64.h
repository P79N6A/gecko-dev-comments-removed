






#ifndef jsion_lir_opcodes_x64_h__
#define jsion_lir_opcodes_x64_h__

#define LIR_CPU_OPCODE_LIST(_)      \
    _(Box)                          \
    _(Unbox)                        \
    _(UnboxDouble)                  \
    _(DivI)                         \
    _(ModI)                         \
    _(ModPowTwoI)                   \
    _(PowHalfD)                     \
    _(UInt32ToDouble)               \
    _(AsmJSLoadFuncPtr)             \
    _(AsmJSDivOrMod)

#endif 

