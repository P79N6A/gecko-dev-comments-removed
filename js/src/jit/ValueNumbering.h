





#ifndef jit_ValueNumbering_h
#define jit_ValueNumbering_h

#include "jit/IonAllocPolicy.h"
#include "js/HashTable.h"

namespace js {
namespace jit {

class MDefinition;
class MBasicBlock;
class MIRGraph;
class MPhi;
class MInstruction;
class MIRGenerator;

class ValueNumberer
{
    
    class VisibleValues
    {
        
        struct ValueHasher
        {
            typedef const MDefinition *Lookup;
            typedef MDefinition *Key;
            static HashNumber hash(Lookup ins);
            static bool match(Key k, Lookup l);
            static void rekey(Key &k, Key newKey);
        };

        typedef HashSet<MDefinition *, ValueHasher, IonAllocPolicy> ValueSet;

        ValueSet set_;        

      public:
        explicit VisibleValues(TempAllocator &alloc);
        bool init();

        typedef ValueSet::Ptr Ptr;
        typedef ValueSet::AddPtr AddPtr;

        Ptr findLeader(const MDefinition *def) const;
        AddPtr findLeaderForAdd(MDefinition *def);
        bool insert(AddPtr p, MDefinition *def);
        void overwrite(AddPtr p, MDefinition *def);
        void forget(const MDefinition *def);
        void clear();
#ifdef DEBUG
        bool has(const MDefinition *def) const;
#endif
    };

    typedef Vector<MBasicBlock *, 4, IonAllocPolicy> BlockWorklist;
    typedef Vector<MDefinition *, 4, IonAllocPolicy> DefWorklist;

    MIRGenerator *const mir_;
    MIRGraph &graph_;
    VisibleValues values_;            
    DefWorklist deadDefs_;            
    BlockWorklist unreachableBlocks_; 
    BlockWorklist remainingBlocks_;   
    size_t numBlocksDeleted_;         
    bool rerun_;                      
    bool blocksRemoved_;              
    bool updateAliasAnalysis_;        
    bool dependenciesBroken_;         

    enum UseRemovedOption {
        DontSetUseRemoved,
        SetUseRemoved
    };

    bool deleteDefsRecursively(MDefinition *def);
    bool discardPhiOperands(MPhi *phi, const MBasicBlock *phiBlock,
                            UseRemovedOption useRemovedOption = SetUseRemoved);
    bool discardInsOperands(MInstruction *ins,
                            UseRemovedOption useRemovedOption = SetUseRemoved);
    bool deleteDef(MDefinition *def,
                   UseRemovedOption useRemovedOption = SetUseRemoved);
    bool processDeadDefs();

    bool removePredecessor(MBasicBlock *block, MBasicBlock *pred);
    bool removeBlocksRecursively(MBasicBlock *block, const MBasicBlock *root);

    MDefinition *simplified(MDefinition *def) const;
    MDefinition *leader(MDefinition *def);
    bool hasLeader(const MPhi *phi, const MBasicBlock *phiBlock) const;
    bool loopHasOptimizablePhi(MBasicBlock *backedge) const;

    bool visitDefinition(MDefinition *def);
    bool visitControlInstruction(MBasicBlock *block, const MBasicBlock *root);
    bool visitBlock(MBasicBlock *block, const MBasicBlock *root);
    bool visitDominatorTree(MBasicBlock *root, size_t *totalNumVisited);
    bool visitGraph();

  public:
    ValueNumberer(MIRGenerator *mir, MIRGraph &graph);

    enum UpdateAliasAnalysisFlag {
        DontUpdateAliasAnalysis,
        UpdateAliasAnalysis,
    };

    
    
    
    bool run(UpdateAliasAnalysisFlag updateAliasAnalysis);
};

} 
} 

#endif 
