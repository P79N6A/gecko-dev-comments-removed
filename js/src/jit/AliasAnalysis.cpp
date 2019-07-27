





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
    LoopAliasInfo *outer_;
    MBasicBlock *loopHeader_;
    MDefinitionVector invariantLoads_;

  public:
    LoopAliasInfo(TempAllocator &alloc, LoopAliasInfo *outer, MBasicBlock *loopHeader)
      : outer_(outer), loopHeader_(loopHeader), invariantLoads_(alloc)
    { }

    MBasicBlock *loopHeader() const {
        return loopHeader_;
    }
    LoopAliasInfo *outer() const {
        return outer_;
    }
    bool addInvariantLoad(MDefinition *ins) {
        return invariantLoads_.append(ins);
    }
    const MDefinitionVector& invariantLoads() const {
        return invariantLoads_;
    }
    MDefinition *firstInstruction() const {
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
    operator bool() const {
        return !!flags;
    }
    unsigned operator *() const {
        JS_ASSERT(pos < AliasSet::NumCategories);
        return pos;
    }
};

} 

AliasAnalysis::AliasAnalysis(MIRGenerator *mir, MIRGraph &graph)
  : mir(mir),
    graph_(graph),
    loop_(nullptr)
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
          case 1: {
            MBasicBlock *successor = src->getSuccessor(0);
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
IonSpewDependency(MDefinition *load, MDefinition *store, const char *verb, const char *reason)
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
IonSpewAliasInfo(const char *pre, MDefinition *ins, const char *post)
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
    Vector<MDefinitionVector, AliasSet::NumCategories, IonAllocPolicy> stores(alloc());

    
    MDefinition *firstIns = *graph_.entryBlock()->begin();
    for (unsigned i = 0; i < AliasSet::NumCategories; i++) {
        MDefinitionVector defs(alloc());
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

                if (JitSpewEnabled(JitSpew_Alias)) {
                    fprintf(JitSpewFile, "Processing store ");
                    def->printName(JitSpewFile);
                    fprintf(JitSpewFile, " (flags %x)\n", set.flags());
                }
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
                IonSpewDependency(*def, lastStore, "depends", "");

                
                
                
                if (loop_ && lastStore->id() < loop_->firstInstruction()->id()) {
                    if (!loop_->addInvariantLoad(*def))
                        return false;
                }
            }
        }

        
        block->lastIns()->setId(newId++);

        if (block->isLoopBackedge()) {
            JS_ASSERT(loop_->loopHeader() == block->loopHeaderOfBackedge());
            JitSpew(JitSpew_Alias, "Processing loop backedge %d (header %d)", block->id(),
                    loop_->loopHeader()->id());
            LoopAliasInfo *outerLoop = loop_->outer();
            MInstruction *firstLoopIns = *loop_->loopHeader()->begin();

            const MDefinitionVector &invariant = loop_->invariantLoads();

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
                            IonSpewDependency(ins, store, "aliases", "store in loop body");
                            break;
                        }
                    }
                    if (hasAlias)
                        break;
                }

                if (hasAlias) {
                    
                    
                    
                    MControlInstruction *controlIns = loop_->loopHeader()->lastIns();
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

    JS_ASSERT(loop_ == nullptr);
    return true;
}
