









































#ifndef jsion_mir_opcodes_h__
#define jsion_mir_opcodes_h__

namespace js {
namespace ion {

#define MIR_OPCODE_LIST(_)                                                  \
    _(Constant)                                                             \
    _(Parameter)                                                            \
    _(Callee)                                                               \
    _(TableSwitch)                                                          \
    _(Goto)                                                                 \
    _(Test)                                                                 \
    _(Compare)                                                              \
    _(Phi)                                                                  \
    _(OsrValue)                                                             \
    _(OsrScopeChain)                                                        \
    _(CheckOverRecursed)                                                    \
    _(RecompileCheck)                                                       \
    _(PrepareCall)                                                          \
    _(PassArg)                                                              \
    _(Call)                                                                 \
    _(BitNot)                                                               \
    _(BitAnd)                                                               \
    _(BitOr)                                                                \
    _(BitXor)                                                               \
    _(Lsh)                                                                  \
    _(Rsh)                                                                  \
    _(Ursh)                                                                 \
    _(Abs)                                                                  \
    _(Add)                                                                  \
    _(Sub)                                                                  \
    _(Mul)                                                                  \
    _(Div)                                                                  \
    _(Mod)                                                                  \
    _(Concat)                                                               \
    _(Return)                                                               \
    _(Throw)                                                                \
    _(Copy)                                                                 \
    _(Box)                                                                  \
    _(Unbox)                                                                \
    _(GuardObject)                                                          \
    _(ToDouble)                                                             \
    _(ToInt32)                                                              \
    _(TruncateToInt32)                                                      \
    _(ToString)                                                             \
    _(NewArray)                                                             \
    _(Start)                                                                \
    _(OsrEntry)                                                             \
    _(RegExp)                                                               \
    _(ImplicitThis)                                                         \
    _(Slots)                                                                \
    _(Elements)                                                             \
    _(LoadSlot)                                                             \
    _(StoreSlot)                                                            \
    _(FunctionEnvironment)                                                  \
    _(TypeBarrier)                                                          \
    _(GetPropertyCache)                                                     \
    _(GuardShape)                                                           \
    _(GuardClass)                                                           \
    _(ArrayLength)                                                          \
    _(InitializedLength)                                                    \
    _(BoundsCheck)                                                          \
    _(BoundsCheckLower)                                                     \
    _(LoadElement)                                                          \
    _(LoadElementHole)                                                      \
    _(StoreElement)                                                         \
    _(StoreElementHole)                                                     \
    _(LoadFixedSlot)                                                        \
    _(StoreFixedSlot)                                                       \
    _(CallGetProperty)                                                      \
    _(CallGetName)                                                          \
    _(CallGetNameTypeOf)                                                    \
    _(CallGetElement)                                                       \
    _(CallSetElement)                                                       \
    _(GenericSetProperty)                                                   \
    _(StringLength)                                                         \
    _(Round)


#define FORWARD_DECLARE(op) class M##op;
 MIR_OPCODE_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class MInstructionVisitor
{
  public:
#define VISIT_INS(op) virtual bool visit##op(M##op *) { JS_NOT_REACHED("NYI: " #op); return false; }
    MIR_OPCODE_LIST(VISIT_INS)
#undef VISIT_INS
};

} 
} 

#endif 

