








































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
    _(NewObject)                    \
    _(CheckOverRecursed)            \
    _(RecompileCheck)               \
    _(DefVar)                       \
    _(CallGeneric)                  \
    _(CallNative)                   \
    _(CallConstructor)              \
    _(StackArg)                     \
    _(CreateThis)                   \
    _(BitNotI)                      \
    _(BitNotV)                      \
    _(BitOpI)                       \
    _(BitOpV)                       \
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
    _(IsNullOrUndefined)            \
    _(IsNullOrUndefinedAndBranch)   \
    _(AbsI)                         \
    _(AbsD)                         \
    _(SqrtD)                        \
    _(NotI)                         \
    _(NotD)                         \
    _(NotV)                         \
    _(AddI)                         \
    _(SubI)                         \
    _(MulI)                         \
    _(MathD)                        \
    _(BinaryV)                      \
    _(Concat)                       \
    _(CharCodeAt)                   \
    _(FromCharCode)                 \
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
    _(ImplicitThis)                 \
    _(Slots)                        \
    _(Elements)                     \
    _(LoadSlotV)                    \
    _(LoadSlotT)                    \
    _(StoreSlotV)                   \
    _(StoreSlotT)                   \
    _(GuardShape)                   \
    _(GuardClass)                   \
    _(TypeBarrier)                  \
    _(MonitorTypes)                 \
    _(InitializedLength)            \
    _(SetInitializedLength)         \
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
    _(LoadTypedArrayElement)        \
    _(LoadTypedArrayElementHole)    \
    _(StoreTypedArrayElement)       \
    _(ClampIToUint8)                \
    _(ClampDToUint8)                \
    _(ClampVToUint8)                \
    _(LoadFixedSlotV)               \
    _(LoadFixedSlotT)               \
    _(StoreFixedSlotV)              \
    _(StoreFixedSlotT)              \
    _(FunctionEnvironment)          \
    _(GetPropertyCacheV)            \
    _(GetPropertyCacheT)            \
    _(GetElementCacheV)             \
    _(BindNameCache)                \
    _(CallGetProperty)              \
    _(CallGetName)                  \
    _(CallGetNameTypeOf)            \
    _(CallGetElement)               \
    _(CallSetElement)               \
    _(CallSetProperty)              \
    _(CallDeleteProperty)           \
    _(SetPropertyCacheV)            \
    _(SetPropertyCacheT)            \
    _(CallIteratorStart)            \
    _(IteratorStart)                \
    _(IteratorNext)                 \
    _(IteratorMore)                 \
    _(IteratorEnd)                  \
    _(ArrayLength)                  \
    _(TypedArrayLength)             \
    _(TypedArrayElements)           \
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

