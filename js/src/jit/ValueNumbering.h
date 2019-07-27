





#ifndef jit_ValueNumbering_h
#define jit_ValueNumbering_h

#include "jit/JitAllocPolicy.h"
#include "js/HashTable.h"

namespace js {
namespace jit {

class MDefinition;
class MBasicBlock;
class MIRGraph;
class MPhi;
class MIRGenerator;
class MResumePoint;

class ValueNumberer
{
    
    class VisibleValues
    {
        
        struct ValueHasher
        {
            typedef const MDefinition* Lookup;
            typedef MDefinition* Key;
            static HashNumber hash(Lookup ins);
            static bool match(Key k, Lookup l);
            static void rekey(Key& k, Key newKey);
        };

        typedef HashSet<MDefinition*, ValueHasher, JitAllocPolicy> ValueSet;

        ValueSet set_;        

      public:
        explicit VisibleValues(TempAllocator& alloc);
        bool init();

        typedef ValueSet::Ptr Ptr;
        typedef ValueSet::AddPtr AddPtr;

        Ptr findLeader(const MDefinition* def) const;
        AddPtr findLeaderForAdd(MDefinition* def);
        bool add(AddPtr p, MDefinition* def);
        void overwrite(AddPtr p, MDefinition* def);
        void forget(const MDefinition* def);
        void clear();
#ifdef DEBUG
        bool has(const MDefinition* def) const;
#endif
    };

    typedef Vector<MBasicBlock*, 4, JitAllocPolicy> BlockWorklist;
    typedef Vector<MDefinition*, 4, JitAllocPolicy> DefWorklist;

    MIRGenerator* const mir_;
    MIRGraph& graph_;
    VisibleValues values_;            
    DefWorklist deadDefs_;            
    BlockWorklist remainingBlocks_;   
    MDefinition* nextDef_;            
    size_t totalNumVisited_;          
    bool rerun_;                      
    bool blocksRemoved_;              
    bool updateAliasAnalysis_;        
    bool dependenciesBroken_;         

    enum UseRemovedOption {
        DontSetUseRemoved,
        SetUseRemoved
    };

    bool handleUseReleased(MDefinition* def, UseRemovedOption useRemovedOption);
    bool discardDefsRecursively(MDefinition* def);
    bool releaseResumePointOperands(MResumePoint* resume);
    bool releaseAndRemovePhiOperands(MPhi* phi);
    bool releaseOperands(MDefinition* def);
    bool discardDef(MDefinition* def);
    bool processDeadDefs();

    bool fixupOSROnlyLoop(MBasicBlock* block, MBasicBlock* backedge);
    bool removePredecessorAndDoDCE(MBasicBlock* block, MBasicBlock* pred, size_t predIndex);
    bool removePredecessorAndCleanUp(MBasicBlock* block, MBasicBlock* pred);

    MDefinition* simplified(MDefinition* def) const;
    MDefinition* leader(MDefinition* def);
    bool hasLeader(const MPhi* phi, const MBasicBlock* phiBlock) const;
    bool loopHasOptimizablePhi(MBasicBlock* header) const;

    bool visitDefinition(MDefinition* def);
    bool visitControlInstruction(MBasicBlock* block, const MBasicBlock* root);
    bool visitUnreachableBlock(MBasicBlock* block);
    bool visitBlock(MBasicBlock* block, const MBasicBlock* root);
    bool visitDominatorTree(MBasicBlock* root);
    bool visitGraph();

  public:
    ValueNumberer(MIRGenerator* mir, MIRGraph& graph);
    bool init();

    enum UpdateAliasAnalysisFlag {
        DontUpdateAliasAnalysis,
        UpdateAliasAnalysis
    };

    
    
    
    bool run(UpdateAliasAnalysisFlag updateAliasAnalysis);
};

} 
} 

#endif 
