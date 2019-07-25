








































#ifndef jsion_include_greedy_h__
#define jsion_include_greedy_h__

#include "MIR.h"
#include "MIRGraph.h"
#include "IonLIR.h"
#include "MoveGroup.h"

namespace js {
namespace ion {

class GreedyAllocator
{
    struct Mover {
        MoveGroup *moves;

        Mover() : moves(NULL)
        { }

        template <typename From, typename To>
        bool move(const From &from, const To &to) {
            if (!moves)
                moves = new MoveGroup;
            return moves->add(LAllocation::New(from), LAllocation::New(to));
        }
    };

    struct VirtualRegister {
        LDefinition *def;
        uint32 stackSlot_;
        union {
            RegisterCodes::Code gprCode;
            FloatRegisterCodes::Code fpuCode;
            uint32 registerCode;
        };
        bool hasRegister_;
        bool hasStackSlot_;

#ifdef DEBUG
        LInstruction *ins;
#endif

        LDefinition::Type type() const {
            return def->type();
        }
        bool isDouble() const {
            return type() == LDefinition::DOUBLE;
        }
        Register gpr() const {
            JS_ASSERT(!isDouble());
            JS_ASSERT(hasRegister());
            return Register::FromCode(gprCode);
        }
        FloatRegister fpu() const {
            JS_ASSERT(isDouble());
            JS_ASSERT(hasRegister());
            return FloatRegister::FromCode(fpuCode);
        }
        AnyRegister reg() const {
            return isDouble() ? AnyRegister(fpu()) : AnyRegister(gpr());
        }
        void setRegister(FloatRegister reg) {
            JS_ASSERT(isDouble());
            fpuCode = reg.code();
            hasRegister_ = true;
        }
        void setRegister(Register reg) {
            JS_ASSERT(!isDouble());
            gprCode = reg.code();
            hasRegister_ = true;
        }
        void setRegister(AnyRegister reg) {
            if (reg.isFloat())
                setRegister(reg.fpu());
            else
                setRegister(reg.gpr());
        }
        uint32 stackSlot() const {
            return stackSlot_;
        }
        bool hasBackingStack() const {
            return hasStackSlot() ||
                   (def->isPreset() && def->output()->isMemory());
        }
        LAllocation backingStack() const {
            if (hasStackSlot())
                return LStackSlot(stackSlot_);
            JS_ASSERT(def->policy() == LDefinition::PRESET);
            JS_ASSERT(def->output()->isMemory());
            return *def->output();
        }
        void setStackSlot(uint32 index) {
            JS_ASSERT(!hasStackSlot());
            stackSlot_ = index;
            hasStackSlot_ = true;
        }
        bool hasRegister() const {
            return hasRegister_;
        }
        void unsetRegister() {
            hasRegister_ = false;
        }
        bool hasSameRegister(uint32 code) const {
            return hasRegister() && registerCode == code;
        }
        bool hasStackSlot() const {
            return hasStackSlot_;
        }
    };

    struct AllocationState {
        RegisterSet free;
        VirtualRegister *gprs[RegisterCodes::Total];
        VirtualRegister *fpus[FloatRegisterCodes::Total];

        VirtualRegister *& operator[](const AnyRegister &reg) {
            if (reg.isFloat())
                return fpus[reg.fpu().code()];
            return gprs[reg.gpr().code()];
        }

        AllocationState()
          : free(RegisterSet::All()),
            gprs(),
            fpus()
        { }
    };

    struct BlockInfo {
        AllocationState in;
        Mover restores;
        Mover phis;
        RegisterSet freeOnExit;
    };

  private:
    MIRGenerator *gen;
    LIRGraph &graph;
    VirtualRegister *vars;
    RegisterSet disallowed;
    RegisterSet discouraged;
    AllocationState state;
    StackAssignment stackSlots;
    BlockInfo *blocks;
    uint32 tempSlot;

    
    
    
    
    
    
    
    
    
    
    
    
    
    MoveGroup *aligns;
    MoveGroup *spills;
    MoveGroup *restores;

    bool restore(const LAllocation &from, const AnyRegister &to) {
        if (!restores)
            restores = new MoveGroup;
        return restores->add(LAllocation::New(from), LAllocation::New(to));
    }

    template <typename LA, typename LB>
    bool spill(const LA &from, const LB &to) {
        if (!spills)
            spills = new MoveGroup;
        return spills->add(LAllocation::New(from), LAllocation::New(to));
    }

    template <typename LA, typename LB>
    bool align(const LA &from, const LB &to) {
        if (!aligns)
            aligns = new MoveGroup;
        return aligns->add(LAllocation::New(from), LAllocation::New(to));
    }

    void reset() {
        aligns = NULL;
        spills = NULL;
        restores = NULL;
        disallowed = RegisterSet();
        discouraged = RegisterSet();
    }

  private:
    void findDefinitionsInLIR(LInstruction *ins);
    void findDefinitionsInBlock(LBlock *block);
    void findDefinitions();

    
    bool kill(VirtualRegister *vr);

    
    
    bool evict(AnyRegister reg);
    bool maybeEvict(AnyRegister reg);

    
    bool allocateStack(VirtualRegister *vr);
    void freeStack(VirtualRegister *vr);

    
    void freeReg(AnyRegister reg);

    
    void assign(VirtualRegister *vr, AnyRegister reg);

    enum Policy {
        
        
        TEMPORARY,

        
        
        DISALLOW
    };

    
    
    bool allocate(LDefinition::Type type, Policy policy, AnyRegister *out);

    
    
    bool allocateRegisterOperand(LAllocation *a, VirtualRegister *vr);
    bool allocateAnyOperand(LAllocation *a, VirtualRegister *vr, bool preferReg = false);
    bool allocateFixedOperand(LAllocation *a, VirtualRegister *vr);

    bool prescanDefinition(LDefinition *def);
    bool prescanDefinitions(LInstruction *ins);
    bool prescanUses(LInstruction *ins);
    bool informSnapshot(LSnapshot *snapshot);
    bool allocateSameAsInput(LDefinition *def, LAllocation *a, AnyRegister *out);
    bool allocateDefinitions(LInstruction *ins);
    bool allocateTemporaries(LInstruction *ins);
    bool allocateInputs(LInstruction *ins);

    bool allocateRegisters();
    bool allocateRegistersInBlock(LBlock *block);
    bool mergePhiState(LBlock *block);
    bool prepareBackedge(LBlock *block);
    bool mergeAllocationState(LBlock *block);
    bool mergeBackedgeState(LBlock *header, LBlock *backedge);
    bool mergeRegisterState(const AnyRegister &reg, LBlock *left, LBlock *right);

    VirtualRegister *getVirtualRegister(LDefinition *def) {
        JS_ASSERT(def->virtualRegister() < graph.numVirtualRegisters());
        return &vars[def->virtualRegister()];
    }
    VirtualRegister *getVirtualRegister(LUse *use) {
        JS_ASSERT(use->virtualRegister() < graph.numVirtualRegisters());
        JS_ASSERT(vars[use->virtualRegister()].def);
        return &vars[use->virtualRegister()];
    }
    RegisterSet allocatableRegs() const {
        return RegisterSet::Intersect(state.free, RegisterSet::Not(disallowed));
    }
    BlockInfo *blockInfo(LBlock *block) {
        return &blocks[block->mir()->id()];
    }

  public:
    GreedyAllocator(MIRGenerator *gen, LIRGraph &graph);

    bool allocate();
};

} 
} 

#endif 

