





#ifndef ion_x86_LOpcodes_x86_h
#define ion_x86_LOpcodes_x86_h

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxDouble)              \
    _(Box)                      \
    _(BoxDouble)                \
    _(DivI)                     \
    _(DivPowTwoI)               \
    _(ModI)                     \
    _(ModPowTwoI)               \
    _(PowHalfD)                 \
    _(UInt32ToDouble)           \
    _(AsmJSLoadFuncPtr)         \
    _(UDivOrMod)

#endif 
