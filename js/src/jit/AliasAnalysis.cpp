





#include "jit/AliasAnalysis.h"

#include <stdio.h>

#include "jit/Ion.h"
#include "jit/IonBuilder.h"
#include "jit/JitSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

using mozilla::Array;

namespace js {
namespace jit {

class LoopAliasInfo : public TempObject
{
  private:
    LoopAliasInfo* outer_;
    MBasicBlock* loopHeader_;
    MInstructionVector invariantLoads_;

  public:
    LoopAliasInfo(TempAllocator& alloc, LoopAliasInfo* outer, MBasicBlock* loopHeader)
      : outer_(outer), loopHeader_(loopHeader), invariantLoads_(alloc)
    { }

    MBasicBlock* loopHeader() const {
        return loopHeader_;
    }
    LoopAliasInfo* outer() const {
        return outer_;
    }
    bool addInvariantLoad(MInstruction* ins) {
        return invariantLoads_.append(ins);
    }
    const MInstructionVector& invariantLoads() const {
        return invariantLoads_;
    }
    MInstruction* firstInstruction() const {
        return *loopHeader_->begin();
    }
};

} 
} 

namespace {


class AliasSetIterator
{
  private:
    uint32_t flags;
    unsigned pos;

  public:
    explicit AliasSetIterator(AliasSet set)
      : flags(set.flags()), pos(0)
    {
        while (flags && (flags & 1) == 0) {
            flags >>= 1;
            pos++;
        }
    }
    AliasSetIterator& operator ++(int) {
        do {
            flags >>= 1;
            pos++;
        } while (flags && (flags & 1) == 0);
        return *this;
    }
    explicit operator bool() const {
        return !!flags;
    }
    unsigned operator*() const {
        MOZ_ASSERT(pos < AliasSet::NumCategories);
        return pos;
    }
};

} 

AliasAnalysis::AliasAnalysis(MIRGenerator* mir, MIRGraph& graph)
  : mir(mir),
    graph_(graph),
    loop_(nullptr)
{
}



static inline bool
BlockMightReach(MBasicBlock* src, MBasicBlock* dest)
{
    while (src->id() <= dest->id()) {
        if (src == dest)
            return true;
        switch (src->numSuccessors()) {
          case 0:
            return false;
          case 1: {
            MBasicBlock* successor = src->getSuccessor(0);
            if (successor->id() <= src->id())
                return true; 
            src = successor;
            break;
          }
          default:
            return true;
        }
    }
    return false;
}

static void
IonSpewDependency(MInstruction* load, MInstruction* store, const char* verb, const char* reason)
{
    if (!JitSpewEnabled(JitSpew_Alias))
        return;

    fprintf(JitSpewFile, "Load ");
    load->printName(JitSpewFile);
    fprintf(JitSpewFile, " %s on store ", verb);
    store->printName(JitSpewFile);
    fprintf(JitSpewFile, " (%s)\n", reason);
}

static void
IonSpewAliasInfo(const char* pre, MInstruction* ins, const char* post)
{
    if (!JitSpewEnabled(JitSpew_Alias))
        return;

    fprintf(JitSpewFile, "%s ", pre);
    ins->printName(JitSpewFile);
    fprintf(JitSpewFile, " %s\n", post);
}















bool
AliasAnalysis::analyze()
{
    Vector<MInstructionVector, AliasSet::NumCategories, JitAllocPolicy> stores(alloc());

    
    MInstruction* firstIns = *graph_.entryBlock()->begin();
    for (unsigned i = 0; i < AliasSet::NumCategories; i++) {
        MInstructionVector defs(alloc());
        if (!defs.append(firstIns))
            return false;
        if (!stores.append(Move(defs)))
            return false;
    }

    
    
    uint32_t newId = 0;

    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (mir->shouldCancel("Alias Analysis (main loop)"))
            return false;

        if (block->isLoopHeader()) {
            JitSpew(JitSpew_Alias, "Processing loop header %d", block->id());
            loop_ = new(alloc()) LoopAliasInfo(alloc(), loop_, *block);
        }

        for (MPhiIterator def(block->phisBegin()), end(block->phisEnd()); def != end; ++def)
            def->setId(newId++);

        for (MInstructionIterator def(block->begin()), end(block->begin(block->lastIns()));
             def != end;
             ++def)
        {
            def->setId(newId++);

            AliasSet set = def->getAliasSet();
            if (set.isNone())
                continue;

            if (set.isStore()) {
                for (AliasSetIterator iter(set); iter; iter++) {
                    if (!stores[*iter].append(*def))
                        return false;
                }

                if (JitSpewEnabled(JitSpew_Alias)) {
                    fprintf(JitSpewFile, "Processing store ");
                    def->printName(JitSpewFile);
                    fprintf(JitSpewFile, " (flags %x)\n", set.flags());
                }
            } else {
                
                MInstruction* lastStore = firstIns;

                for (AliasSetIterator iter(set); iter; iter++) {
                    MInstructionVector& aliasedStores = stores[*iter];
                    for (int i = aliasedStores.length() - 1; i >= 0; i--) {
                        MInstruction* store = aliasedStores[i];
                        if (def->mightAlias(store) && BlockMightReach(store->block(), *block)) {
                            if (lastStore->id() < store->id())
                                lastStore = store;
                            break;
                        }
                    }
                }

                def->setDependency(lastStore);
                IonSpewDependency(*def, lastStore, "depends", "");

                
                
                
                if (loop_ && lastStore->id() < loop_->firstInstruction()->id()) {
                    if (!loop_->addInvariantLoad(*def))
                        return false;
                }
            }
        }

        
        block->lastIns()->setId(newId++);

        if (block->isLoopBackedge()) {
            MOZ_ASSERT(loop_->loopHeader() == block->loopHeaderOfBackedge());
            JitSpew(JitSpew_Alias, "Processing loop backedge %d (header %d)", block->id(),
                    loop_->loopHeader()->id());
            LoopAliasInfo* outerLoop = loop_->outer();
            MInstruction* firstLoopIns = *loop_->loopHeader()->begin();

            const MInstructionVector& invariant = loop_->invariantLoads();

            for (unsigned i = 0; i < invariant.length(); i++) {
                MInstruction* ins = invariant[i];
                AliasSet set = ins->getAliasSet();
                MOZ_ASSERT(set.isLoad());

                bool hasAlias = false;
                for (AliasSetIterator iter(set); iter; iter++) {
                    MInstructionVector& aliasedStores = stores[*iter];
                    for (int i = aliasedStores.length() - 1;; i--) {
                        MInstruction* store = aliasedStores[i];
                        if (store->id() < firstLoopIns->id())
                            break;
                        if (ins->mightAlias(store)) {
                            hasAlias = true;
                            IonSpewDependency(ins, store, "aliases", "store in loop body");
                            break;
                        }
                    }
                    if (hasAlias)
                        break;
                }

                if (hasAlias) {
                    
                    
                    
                    MControlInstruction* controlIns = loop_->loopHeader()->lastIns();
                    IonSpewDependency(ins, controlIns, "depends", "due to stores in loop body");
                    ins->setDependency(controlIns);
                } else {
                    IonSpewAliasInfo("Load", ins, "does not depend on any stores in this loop");

                    if (outerLoop && ins->dependency()->id() < outerLoop->firstInstruction()->id()) {
                        IonSpewAliasInfo("Load", ins, "may be invariant in outer loop");
                        if (!outerLoop->addInvariantLoad(ins))
                            return false;
                    }
                }
            }
            loop_ = loop_->outer();
        }
    }

    MOZ_ASSERT(loop_ == nullptr);
    return true;
}
