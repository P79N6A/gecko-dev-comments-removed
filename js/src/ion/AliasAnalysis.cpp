






#include <stdio.h>

#include "MIR.h"
#include "AliasAnalysis.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonBuilder.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;


class AliasSetIterator
{
  private:
    uint32_t flags;
    unsigned pos;

  public:
    AliasSetIterator(AliasSet set)
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
    operator bool() const {
        return !!flags;
    }
    unsigned operator *() const {
        JS_ASSERT(pos < AliasSet::NumCategories);
        return pos;
    }
};

AliasAnalysis::AliasAnalysis(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir),
    graph_(graph),
    loop_(NULL)
{
}



static inline bool
BlockMightReach(MBasicBlock *src, MBasicBlock *dest)
{
    while (src->id() <= dest->id()) {
        if (src == dest)
            return true;
        switch (src->numSuccessors()) {
          case 0:
            return false;
          case 1:
            src = src->getSuccessor(0);
            break;
          default:
            return true;
        }
    }
    return false;
}















bool
AliasAnalysis::analyze()
{
    FixedArityList<MDefinitionVector, AliasSet::NumCategories> stores;

    
    MDefinition *firstIns = *graph_.begin()->begin();
    for (unsigned i=0; i < AliasSet::NumCategories; i++) {
        if (!stores[i].append(firstIns))
            return false;
    }

    
    
    
    uint32_t newId = 1;

    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (mir->shouldCancel("Alias Analysis (main loop)"))
            return false;

        if (block->isLoopHeader()) {
            IonSpew(IonSpew_Alias, "Processing loop header %d", block->id());
            loop_ = new LoopAliasInfo(loop_, *block);
        }

        for (MDefinitionIterator def(*block); def; def++) {
            def->setId(newId++);

            AliasSet set = def->getAliasSet();
            if (set.isNone())
                continue;

            if (set.isStore()) {
                for (AliasSetIterator iter(set); iter; iter++) {
                    if (!stores[*iter].append(*def))
                        return false;
                }

                IonSpew(IonSpew_Alias, "Processing store %d (flags %x)", def->id(), set.flags());
            } else {
                
                MDefinition *lastStore = firstIns;

                for (AliasSetIterator iter(set); iter; iter++) {
                    MDefinitionVector &aliasedStores = stores[*iter];
                    for (int i = aliasedStores.length() - 1; i >= 0; i--) {
                        MDefinition *store = aliasedStores[i];
                        if (def->mightAlias(store) && BlockMightReach(store->block(), *block)) {
                            if (lastStore->id() < store->id())
                                lastStore = store;
                            break;
                        }
                    }
                }

                def->setDependency(lastStore);
                IonSpew(IonSpew_Alias, "Load %d depends on store %d", def->id(), lastStore->id());

                
                
                
                if (loop_ && lastStore->id() < loop_->firstInstruction()->id()) {
                    if (!loop_->addInvariantLoad(*def))
                        return false;
                }
            }
        }

        if (block->isLoopBackedge()) {
            JS_ASSERT(loop_->loopHeader() == block->loopHeaderOfBackedge());
            IonSpew(IonSpew_Alias, "Processing loop backedge %d (header %d)", block->id(),
                    loop_->loopHeader()->id());
            LoopAliasInfo *outerLoop = loop_->outer();
            MInstruction *firstLoopIns = *loop_->loopHeader()->begin();

            const InstructionVector &invariant = loop_->invariantLoads();

            for (unsigned i = 0; i < invariant.length(); i++) {
                MDefinition *ins = invariant[i];
                AliasSet set = ins->getAliasSet();
                JS_ASSERT(set.isLoad());

                bool hasAlias = false;
                for (AliasSetIterator iter(set); iter; iter++) {
                    MDefinitionVector &aliasedStores = stores[*iter];
                    for (int i = aliasedStores.length() - 1;; i--) {
                        MDefinition *store = aliasedStores[i];
                        if (store->id() < firstLoopIns->id())
                            break;
                        if (ins->mightAlias(store)) {
                            hasAlias = true;
                            break;
                        }
                    }
                    if (hasAlias)
                        break;
                }

                if (hasAlias) {
                    
                    
                    
                    MControlInstruction *controlIns = loop_->loopHeader()->lastIns();
                    IonSpew(IonSpew_Alias, "Load %d depends on %d (due to stores in loop body)",
                            ins->id(), controlIns->id());
                    ins->setDependency(controlIns);
                } else {
                    IonSpew(IonSpew_Alias, "Load %d does not depend on any stores in this loop",
                            ins->id());

                    if (outerLoop && ins->dependency()->id() < outerLoop->firstInstruction()->id()) {
                        IonSpew(IonSpew_Alias, "Load %d may be invariant in outer loop", ins->id());
                        if (!outerLoop->addInvariantLoad(ins))
                            return false;
                    }
                }
            }
            loop_ = loop_->outer();
        }
    }

    JS_ASSERT(loop_ == NULL);
    return true;
}
