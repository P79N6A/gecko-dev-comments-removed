







































#include <stdio.h>

#include "MIR.h"
#include "AliasAnalysis.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;


class AliasSetIterator
{
  private:
    uint32 flags;
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
        return pos;
    }
};

AliasAnalysis::AliasAnalysis(MIRGraph &graph)
  : graph_(graph),
    loop_(NULL)
{
}















bool
AliasAnalysis::analyze()
{
    Vector<MDefinition *, 16, SystemAllocPolicy> stores;

    
    MDefinition *firstIns = *graph_.begin()->begin();
    for (unsigned i=0; i < NUM_ALIAS_SETS; i++) {
        if (!stores.append(firstIns))
            return false;
    }

    
    
    
    uint32 newId = 1;

    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
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
                for (AliasSetIterator iter(set); iter; iter++)
                    stores[*iter] = *def;

                IonSpew(IonSpew_Alias, "Processing store %d (flags %x)", def->id(), set.flags());

                if (loop_)
                    loop_->addStore(set);
            } else {
                
                MDefinition *lastStore = NULL;

                for (AliasSetIterator iter(set); iter; iter++) {
                    MDefinition *store = stores[*iter];
                    if (!lastStore || lastStore->id() < store->id())
                        lastStore = store;
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

            
            if (outerLoop)
                outerLoop->addStore(loop_->loopStores());

            const InstructionVector &invariant = loop_->invariantLoads();

            for (unsigned i = 0; i < invariant.length(); i++) {
                MDefinition *ins = invariant[i];
                AliasSet set = ins->getAliasSet();
                JS_ASSERT(set.isLoad());

                if ((loop_->loopStores() & set).isNone()) {
                    IonSpew(IonSpew_Alias, "Load %d does not depend on any stores in this loop",
                            ins->id());

                    if (outerLoop && ins->dependency()->id() < outerLoop->firstInstruction()->id()) {
                        IonSpew(IonSpew_Alias, "Load %d may be invariant in outer loop", ins->id());
                        if (!outerLoop->addInvariantLoad(ins))
                            return false;
                    }
                } else {
                    
                    
                    
                    MControlInstruction *controlIns = loop_->loopHeader()->lastIns();
                    IonSpew(IonSpew_Alias, "Load %d depends on %d (due to stores in loop body)",
                            ins->id(), controlIns);
                    ins->setDependency(controlIns);
                }
            }
            loop_ = loop_->outer();
        }
    }

    JS_ASSERT(loop_ == NULL);
    return true;
}
