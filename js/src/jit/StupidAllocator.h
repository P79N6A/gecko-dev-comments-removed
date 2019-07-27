





#ifndef jit_StupidAllocator_h
#define jit_StupidAllocator_h

#include "jit/RegisterAllocator.h"



namespace js {
namespace jit {

class StupidAllocator : public RegisterAllocator
{
    static const uint32_t MAX_REGISTERS = AnyRegister::Total;
    static const uint32_t MISSING_ALLOCATION = UINT32_MAX;

    struct AllocatedRegister {
        AnyRegister reg;

        
        LDefinition::Type type;

        
        uint32_t vreg;

        
        uint32_t age;

        
        bool dirty;

        void set(uint32_t vreg, LInstruction *ins = nullptr, bool dirty = false) {
            this->vreg = vreg;
            this->age = ins ? ins->id() : 0;
            this->dirty = dirty;
        }
    };

    
    mozilla::Array<AllocatedRegister, MAX_REGISTERS> registers;
    uint32_t registerCount;

    
    typedef uint32_t RegisterIndex;

    
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

    LAllocation *stackLocation(uint32_t vreg);

    RegisterIndex registerIndex(AnyRegister reg);

    AnyRegister ensureHasRegister(LInstruction *ins, uint32_t vreg);
    RegisterIndex allocateRegister(LInstruction *ins, uint32_t vreg);

    void syncRegister(LInstruction *ins, RegisterIndex index);
    void evictRegister(LInstruction *ins, RegisterIndex index);
    void evictAliasedRegister(LInstruction *ins, RegisterIndex index);
    void loadRegister(LInstruction *ins, uint32_t vreg, RegisterIndex index, LDefinition::Type type);

    RegisterIndex findExistingRegister(uint32_t vreg);

    bool allocationRequiresRegister(const LAllocation *alloc, AnyRegister reg);
    bool registerIsReserved(LInstruction *ins, AnyRegister reg);
};

} 
} 

#endif 
