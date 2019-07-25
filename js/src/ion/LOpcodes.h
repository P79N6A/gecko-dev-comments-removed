








































#ifndef jsion_lir_opcodes_common_h__
#define jsion_lir_opcodes_common_h__

#define LIR_COMMON_OPCODE_LIST(_)   \
    _(Label)                        \
    _(Nop)                          \
    _(OsiPoint)                     \
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
    _(DefVar)                       \
    _(CallGeneric)                  \
    _(CallNative)                   \
    _(StackArg)                     \
    _(BitNotI)                      \
    _(BitNotV)                      \
    _(BitOp)                        \
    _(ShiftOp)                      \
    _(Return)                       \
    _(Throw)                        \
    _(Phi)                          \
    _(TestIAndBranch)               \
    _(TestDAndBranch)               \
    _(TestVAndBranch)               \
    _(Compare)                      \
    _(CompareD)                     \
    _(CompareV)                     \
    _(CompareAndBranch)             \
    _(CompareDAndBranch)            \
    _(AbsI)                         \
    _(AbsD)                         \
    _(NotI)                         \
    _(NotD)                         \
    _(NotV)                         \
    _(AddI)                         \
    _(SubI)                         \
    _(MulI)                         \
    _(MathD)                        \
    _(BinaryV)                      \
    _(Concat)                       \
    _(Int32ToDouble)                \
    _(ValueToDouble)                \
    _(ValueToInt32)                 \
    _(DoubleToInt32)                \
    _(TruncateDToInt32)             \
    _(IntToString)                  \
    _(Start)                        \
    _(OsrEntry)                     \
    _(OsrValue)                     \
    _(OsrScopeChain)                \
    _(RegExp)                       \
    _(Lambda)                       \
    _(LambdaJoinableForCall)        \
    _(LambdaJoinableForSet)         \
    _(ImplicitThis)                 \
    _(Slots)                        \
    _(Elements)                     \
    _(FlatClosureUpvars)            \
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
    _(BoundsCheckRange)             \
    _(BoundsCheckLower)             \
    _(LoadElementV)                 \
    _(LoadElementT)                 \
    _(LoadElementHole)              \
    _(StoreElementV)                \
    _(StoreElementT)                \
    _(StoreElementHoleV)            \
    _(StoreElementHoleT)            \
    _(LoadFixedSlotV)               \
    _(LoadFixedSlotT)               \
    _(StoreFixedSlotV)              \
    _(StoreFixedSlotT)              \
    _(FunctionEnvironment)          \
    _(GetPropertyCacheV)            \
    _(GetPropertyCacheT)            \
    _(CallGetProperty)              \
    _(CallGetName)                  \
    _(CallGetNameTypeOf)            \
    _(CallGetElement)               \
    _(CallSetElement)               \
    _(CallSetProperty)              \
    _(SetPropertyCacheV)            \
    _(SetPropertyCacheT)            \
    _(CallIteratorStart)            \
    _(CallIteratorNext)             \
    _(CallIteratorMore)             \
    _(CallIteratorEnd)              \
    _(ArrayLength)                  \
    _(StringLength)                 \
    _(TypeOfV)                      \
    _(ToIdV)                        \
    _(Round)

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

