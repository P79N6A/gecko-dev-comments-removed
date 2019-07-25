








































#ifndef jsion_lir_opcodes_common_h__
#define jsion_lir_opcodes_common_h__

#define LIR_COMMON_OPCODE_LIST(_)   \
    _(Label)                        \
    _(CaptureAllocations)           \
    _(MoveGroup)                    \
    _(Integer)                      \
    _(Pointer)                      \
    _(Double)                       \
    _(Value)                        \
    _(Parameter)                    \
    _(Callee)                       \
    _(TableSwitch)                  \
    _(Goto)                         \
    _(NewArray)                     \
    _(CheckOverRecursed)            \
    _(RecompileCheck)               \
    _(CallGeneric)                  \
    _(StackArg)                     \
    _(BitNot)                       \
    _(BitOp)                        \
    _(ShiftOp)                      \
    _(Return)                       \
    _(Phi)                          \
    _(TestIAndBranch)               \
    _(TestDAndBranch)               \
    _(TestVAndBranch)               \
    _(CompareI)                     \
    _(CompareD)                     \
    _(CompareIAndBranch)            \
    _(CompareDAndBranch)            \
    _(AddI)                         \
    _(SubI)                         \
    _(MulI)                         \
    _(MathD)                        \
    _(Int32ToDouble)                \
    _(ValueToDouble)                \
    _(ValueToInt32)                 \
    _(DoubleToInt32)                \
    _(TruncateDToInt32)             \
    _(Start)                        \
    _(OsrEntry)                     \
    _(OsrValue)                     \
    _(OsrScopeChain)                \
    _(ImplicitThis)                 \
    _(Slots)                        \
    _(Elements)                     \
    _(LoadSlotV)                    \
    _(LoadSlotT)                    \
    _(StoreSlotV)                   \
    _(StoreSlotT)                   \
    _(GuardShape)                   \
    _(GuardClass)                   \
    _(WriteBarrierV)                \
    _(WriteBarrierT)                \
    _(TypeBarrier)                  \
    _(InitializedLength)            \
    _(BoundsCheck)                  \
    _(LoadElementV)                 \
    _(LoadElementT)                 \
    _(StoreElementV)                \
    _(StoreElementT)                \
    _(FunctionEnvironment)          \
    _(GetPropertyCacheV)            \
    _(GetPropertyCacheT)            \
    _(CallGetProperty)              \
    _(CallGetName)                  \
    _(CallGetNameTypeOf)            \
    _(ArrayLength)                  \
    _(StringLength)

#if defined(JS_CPU_X86)
# include "x86/LOpcodes-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/LOpcodes-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/LOpcodes-arm.h"
#endif

#define LIR_OPCODE_LIST(_)          \
    LIR_COMMON_OPCODE_LIST(_)       \
    LIR_CPU_OPCODE_LIST(_)

#endif 

