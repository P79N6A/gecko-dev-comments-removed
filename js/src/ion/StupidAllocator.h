






#ifndef js_ion_stupidallocator_h__
#define js_ion_stupidallocator_h__

#include "RegisterAllocator.h"



namespace js {
namespace ion {

class StupidAllocator : public RegisterAllocator
{
    static const uint32 MAX_REGISTERS = Registers::Allocatable + FloatRegisters::Allocatable;
    static const uint32 MISSING_ALLOCATION = UINT32_MAX;

    struct AllocatedRegister {
        AnyRegister reg;

        
        uint32 vreg;

        
        uint32 age;

        
        bool dirty;

        void set(uint32 vreg, LInstruction *ins = NULL, bool dirty = false) {
            this->vreg = vreg;
            this->age = ins ? ins->id() : 0;
            this->dirty = dirty;
        }
    };

    
    AllocatedRegister registers[MAX_REGISTERS];
    uint32 registerCount;

    
    typedef uint32 RegisterIndex;

    
    Vector<LDefinition*, 0, SystemAllocPolicy> virtualRegisters;

  public:
    StupidAllocator(MIRGenerator *mir, LIRGenerator *lir, LIRGraph &graph)
      : RegisterAllocator(mir, lir, graph)
    {
    }

    bool go();

  private:
    bool init();

    void syncForBlockEnd(LBlock *block, LInstruction *ins);
    void allocateForInstruction(LInstruction *ins);
    void allocateForDefinition(LInstruction *ins, LDefinition *def);

    LAllocation *stackLocation(uint32 vreg);

    RegisterIndex registerIndex(AnyRegister reg);

    AnyRegister ensureHasRegister(LInstruction *ins, uint32 vreg);
    RegisterIndex allocateRegister(LInstruction *ins, uint32 vreg);

    void syncRegister(LInstruction *ins, RegisterIndex index);
    void evictRegister(LInstruction *ins, RegisterIndex index);
    void loadRegister(LInstruction *ins, uint32 vreg, RegisterIndex index);

    RegisterIndex findExistingRegister(uint32 vreg);
};

} 
} 

#endif
