









































#ifndef jsion_mir_opcodes_h__
#define jsion_mir_opcodes_h__

namespace js {
namespace ion {

#define MIR_OPCODE_LIST(_)                                                  \
    _(Constant)                                                             \
    _(Parameter)                                                            \
    _(TableSwitch)                                                          \
    _(Goto)                                                                 \
    _(Test)                                                                 \
    _(Compare)                                                              \
    _(Phi)                                                                  \
    _(OsrValue)                                                             \
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
    _(Add)                                                                  \
    _(Sub)                                                                  \
    _(Mul)                                                                  \
    _(Div)                                                                  \
    _(Return)                                                               \
    _(Copy)                                                                 \
    _(Box)                                                                  \
    _(Unbox)                                                                \
    _(ToDouble)                                                             \
    _(ToInt32)                                                              \
    _(TruncateToInt32)                                                      \
    _(NewArray)                                                             \
    _(Start)                                                                \
    _(OsrEntry)                                                             \
    _(ImplicitThis)                                                         \
    _(Slots)                                                                \
    _(Elements)                                                             \
    _(LoadSlot)                                                             \
    _(StoreSlot)                                                            \
    _(TypeBarrier)                                                          \
    _(GuardShape)                                                           \
    _(GuardClass)                                                           \
    _(InitializedLength)                                                    \
    _(BoundsCheck)                                                          \
    _(LoadElement)                                                          \
    _(StoreElement)                                                         \
    _(LoadProperty)


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

