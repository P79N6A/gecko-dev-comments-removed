








































#ifndef jsion_include_greedy_h__
#define jsion_include_greedy_h__

#include "MIR.h"
#include "MIRGraph.h"
#include "LIR.h"
#include "StackSlotAllocator.h"

namespace js {
namespace ion {

class GreedyAllocator
{
    struct Mover {
        LMoveGroup *moves;

        Mover() : moves(NULL)
        { }

        template <typename From, typename To>
        bool move(const From &from, const To &to) {
            if (!moves)
                moves = new LMoveGroup;
            return moves->add(LAllocation::New(from), LAllocation::New(to));
        }
    };

    struct VirtualRegister : public InlineListNode<VirtualRegister>
    {
        LDefinition *def;
        uint32 stackSlot_;
        union {
            Registers::Code gprCode;
            FloatRegisters::Code fpuCode;
            uint32 registerCode;
        };
        bool hasRegister_;
        bool hasStackSlot_;
        bool hasBackingStack_;
        mutable bool backingStackUsed_;

        
        
        
        bool ownsStackSlot_;

#ifdef DEBUG
        LInstruction *ins;
#endif

        void define(LDefinition *def, LInstruction *ins) {
            this->def = def;
            this->hasBackingStack_ = (def->isPreset() && def->output()->isMemory());
#ifdef DEBUG
            this->ins = ins;
#endif
        }
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
            return hasStackSlot() || hasBackingStack_;
        }
        bool backingStackUsed() const {
            return backingStackUsed_;
        }
        LAllocation backingStack() const {
            backingStackUsed_ = true;
            if (hasStackSlot())
                return LStackSlot(stackSlot_, isDouble());
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
        void setOwnsStackSlot() {
            ownsStackSlot_ = true;
        }
        bool ownsStackSlot() const {
            return ownsStackSlot_;
        }
    };

    struct AllocationState {
        RegisterSet free;
        VirtualRegister *gprs[Registers::Total];
        VirtualRegister *fpus[FloatRegisters::Total];

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

  private:
    MIRGenerator *gen;
    LIRGraph &graph;
    VirtualRegister *vars;
    RegisterSet disallowed;
    RegisterSet discouraged;
    AllocationState state;
    StackSlotAllocator stackSlotAllocator;
    InlineList<VirtualRegister> liveSlots_;

    
    
    
    
    
    
    
    
    
    
    
    
    
    Mover phiMoves;
    LMoveGroup *aligns;
    LMoveGroup *spills;
    LMoveGroup *restores;

    bool restore(const LAllocation &from, const AnyRegister &to) {
        if (!restores)
            restores = new LMoveGroup;
        return restores->add(LAllocation::New(from), LAllocation::New(to));
    }

    template <typename LA, typename LB>
    bool spill(const LA &from, const LB &to) {
        if (!spills)
            spills = new LMoveGroup;
        return spills->add(LAllocation::New(from), LAllocation::New(to));
    }

    template <typename LA, typename LB>
    bool align(const LA &from, const LB &to) {
        if (!aligns)
            aligns = new LMoveGroup;
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
    void assertValidRegisterState();

    void findDefinitionsInLIR(LInstruction *ins);
    void findDefinitionsInBlock(LBlock *block);
    void findDefinitions();

    
    void killReg(VirtualRegister *vr);
    void killStack(VirtualRegister *vr);

    
    
    bool evict(AnyRegister reg);
    bool maybeEvict(AnyRegister reg);

    
    uint32 allocateSlotFor(VirtualRegister *vr);
    void allocateStack(VirtualRegister *vr);
    void freeStack(VirtualRegister *vr);

    
    void freeReg(AnyRegister reg);

    
    void assign(VirtualRegister *vr, AnyRegister reg);

    enum Policy {
        
        
        TEMPORARY,

        
        
        DISALLOW
    };

    
    
    bool allocate(LDefinition::Type type, Policy policy, AnyRegister *out);
    bool allocateReg(VirtualRegister *vreg);

    
    
    bool allocateRegisterOperand(LAllocation *a, VirtualRegister *vr);
    bool allocateAnyOperand(LAllocation *a, VirtualRegister *vr, bool preferReg = false);
    bool allocateFixedOperand(LAllocation *a, VirtualRegister *vr);

    bool prescanDefinition(LDefinition *def);
    bool prescanDefinitions(LInstruction *ins);
    bool prescanUses(LInstruction *ins);
    bool spillForCall(LInstruction *ins);
    void informSnapshot(LInstruction *ins);
    bool informSafepoint(LSafepoint *safepoint);
    bool allocateDefinitions(LInstruction *ins);
    bool allocateDefinition(LInstruction *ins, LDefinition *def);
    bool spillDefinition(LDefinition *def);
    bool allocateTemporaries(LInstruction *ins);
    bool allocateInputs(LInstruction *ins);

    bool allocateRegisters();
    bool allocateRegistersInBlock(LBlock *block);
    bool allocateInstruction(LBlock *block, LInstruction *ins);
    bool buildPhiMoves(LBlock *block);
    bool findLoopCarriedUses(LBlock *block);
    void findLoopCarriedUses(LInstruction *ins, uint32 lowerBound, uint32 upperBound);
    void findLoopCarriedUses(LAllocation *a, uint32 lowerBound, uint32 upperBound);

    VirtualRegister *otherHalfOfNunbox(VirtualRegister *vreg);
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

  public:
    GreedyAllocator(MIRGenerator *gen, LIRGraph &graph);

    bool allocate();
};

} 
} 

#endif 

