








































#ifndef jsion_move_resolver_x86_shared_h__
#define jsion_move_resolver_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveResolverX86
{
    bool inCycle_;
    CodeGenerator *codegen;

    RegisterSet freeRegs_;

    
    
    
    int32 pushedAtCycle_;
    int32 pushedAtSpill_;
    int32 pushedAtDoubleSpill_;

    
    
    
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    
    Register cycleReg_;
    FloatRegister cycleFloatReg_;

    void assertDone();
    void assertValidMove(const LAllocation *from, const LAllocation *to);
    Register tempReg();
    FloatRegister tempFloatReg();
    Operand cycleSlot() const;
    Operand spillSlot() const;
    Operand doubleSpillSlot() const;

    void emitMove(const LAllocation *from, const LAllocation *to);
    void emitDoubleMove(const LAllocation *from, const LAllocation *to);
    void breakCycle(const LAllocation *from, const LAllocation *to);
    void completeCycle(const LAllocation *from, const LAllocation *to);

  public:
    MoveResolverX86(CodeGenerator *codegen);
    ~MoveResolverX86();
    FloatRegister reserveDouble();
    void setup(LMoveGroup *group);
    void emit(const MoveGroupResolver::Move &move);
    void finish();
};

} 
} 

#endif 

