





#ifndef jit_arm_LOpcodes_arm_h
#define jit_arm_LOpcodes_arm_h

#define LIR_CPU_OPCODE_LIST(_)  \
    _(Unbox)                    \
    _(UnboxDouble)              \
    _(Box)                      \
    _(BoxDouble)                \
    _(DivI)                     \
    _(SoftDivI)                 \
    _(DivPowTwoI)               \
    _(ModI)                     \
    _(SoftModI)                 \
    _(ModPowTwoI)               \
    _(ModMaskI)                 \
    _(PowHalfD)                 \
    _(UInt32ToDouble)           \
    _(UDiv)                     \
    _(UMod)                     \
    _(SoftUDivOrMod)            \
    _(AsmJSLoadFuncPtr)

#endif 
