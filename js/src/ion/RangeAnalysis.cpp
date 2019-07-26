





#include <stdio.h>

#include "Ion.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "RangeAnalysis.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;



























































RangeAnalysis::RangeAnalysis(MIRGraph &graph)
  : graph_(graph)
{
}

static bool
IsDominatedUse(MBasicBlock *block, MUse *use)
{
    MNode *n = use->node();
    bool isPhi = n->isDefinition() && n->toDefinition()->isPhi();

    if (isPhi)
        return block->dominates(n->block()->getPredecessor(use->index()));

    return block->dominates(n->block());
}

static inline void
SpewRange(MDefinition *def)
{
#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Range)) {
        IonSpewHeader(IonSpew_Range);
        fprintf(IonSpewFile, "%d has range ", def->id());
        def->range()->printRange(IonSpewFile);
        fprintf(IonSpewFile, "\n");
    }
#endif
}

void
RangeAnalysis::replaceDominatedUsesWith(MDefinition *orig, MDefinition *dom,
                                            MBasicBlock *block)
{
    for (MUseIterator i(orig->usesBegin()); i != orig->usesEnd(); ) {
        if (i->node() != dom && IsDominatedUse(block, *i))
            i = i->node()->replaceOperand(i, dom);
        else
            i++;
    }
}

bool
RangeAnalysis::addBetaNobes()
{
    IonSpew(IonSpew_Range, "Adding beta nobes");

    for (PostorderIterator i(graph_.poBegin()); i != graph_.poEnd(); i++) {
        MBasicBlock *block = *i;
        IonSpew(IonSpew_Range, "Looking at block %d", block->id());

        BranchDirection branch_dir;
        MTest *test = block->immediateDominatorBranch(&branch_dir);

        if (!test || !test->getOperand(0)->isCompare())
            continue;

        MCompare *compare = test->getOperand(0)->toCompare();

        MDefinition *left = compare->getOperand(0);
        MDefinition *right = compare->getOperand(1);
        int32 bound;
        MDefinition *val = NULL;

        JSOp jsop = compare->jsop();

        if (branch_dir == FALSE_BRANCH)
            jsop = analyze::NegateCompareOp(jsop);

        if (left->isConstant() && left->toConstant()->value().isInt32()) {
            bound = left->toConstant()->value().toInt32();
            val = right;
            jsop = analyze::ReverseCompareOp(jsop);
        } else if (right->isConstant() && right->toConstant()->value().isInt32()) {
            bound = right->toConstant()->value().toInt32();
            val = left;
        } else {
            MDefinition *smaller = NULL;
            MDefinition *greater = NULL;
            if (jsop == JSOP_LT) {
                smaller = left;
                greater = right;
            } else if (JSOP_GT) {
                smaller = right;
                greater = left;
            }
            if (smaller && greater) {
                MBeta *beta;
                beta = MBeta::New(smaller, JSVAL_INT_MIN, JSVAL_INT_MAX-1);
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(smaller, beta, block);
                beta = MBeta::New(greater, JSVAL_INT_MIN+1, JSVAL_INT_MAX);
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(greater, beta, block);
            }
            continue;
        }

        JS_ASSERT(val);


        int32 low = JSVAL_INT_MIN;
        int32 high = JSVAL_INT_MAX;
        switch (jsop) {
          case JSOP_LE:
            high = bound;
            break;
          case JSOP_LT:
            if (!SafeSub(bound, 1, &bound))
                break;
            high = bound;
            break;
          case JSOP_GE:
            low = bound;
            break;
          case JSOP_GT:
            if (!SafeAdd(bound, 1, &bound))
                break;
            low = bound;
            break;
          case JSOP_EQ:
            low = bound;
            high = bound;
          default:
            break; 
                   
        }

        IonSpew(IonSpew_Range, "Adding beta node for %d", val->id());
        MBeta *beta = MBeta::New(val, low, high);
        block->insertBefore(*block->begin(), beta);
        replaceDominatedUsesWith(val, beta, block);
    }

    return true;
}

bool
RangeAnalysis::removeBetaNobes()
{
    IonSpew(IonSpew_Range, "Removing beta nobes");

    for (PostorderIterator i(graph_.poBegin()); i != graph_.poEnd(); i++) {
        MBasicBlock *block = *i;
        for (MDefinitionIterator iter(*i); iter; ) {
            MDefinition *def = *iter;
            if (def->isBeta()) {
                MDefinition *op = def->getOperand(0);
                IonSpew(IonSpew_Range, "Removing beta node %d for %d",
                        def->id(), op->id());
                def->replaceAllUsesWith(op);
                iter = block->discardDefAt(iter);
            } else {
                
                
                
                break;
            }
        }
    }
    return true;
}

void
Range::printRange(FILE *fp)
{
    JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
    JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    fprintf(fp, "[");
    if (lower_infinite_) { fprintf(fp, "-inf"); } else { fprintf(fp, "%d", lower_); }
    fprintf(fp, ", ");
    if (upper_infinite_) { fprintf(fp, "inf"); } else { fprintf(fp, "%d", upper_); }
    fprintf(fp, "]");
}

Range
Range::intersect(const Range *lhs, const Range *rhs)
{
    Range r(
        Max(lhs->lower_, rhs->lower_),
        Min(lhs->upper_, rhs->upper_));

    r.lower_infinite_ = lhs->lower_infinite_ && rhs->lower_infinite_;
    r.upper_infinite_ = lhs->upper_infinite_ && rhs->upper_infinite_;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (r.upper_ < r.lower_)
        r.makeRangeInfinite();
    return r;
}

void
Range::unionWith(const Range *other)
{
    setLower(Min(lower_, other->lower_));
    setUpper(Max(upper_, other->upper_));
    lower_infinite_ |= other->lower_infinite_;
    upper_infinite_ |= other->upper_infinite_;
}

Range
Range::add(const Range *lhs, const Range *rhs)
{
    return Range(
        (int64_t)lhs->lower_ + (int64_t)rhs->lower_,
        (int64_t)lhs->upper_ + (int64_t)rhs->upper_);
}

Range
Range::sub(const Range *lhs, const Range *rhs)
{
    return Range(
        (int64_t)lhs->lower_ - (int64_t)rhs->upper_,
        (int64_t)lhs->upper_ - (int64_t)rhs->lower_);
}

Range
Range::mul(const Range *lhs, const Range *rhs)
{
    int64_t a = (int64_t)lhs->lower_ * (int64_t)rhs->lower_;
    int64_t b = (int64_t)lhs->lower_ * (int64_t)rhs->upper_;
    int64_t c = (int64_t)lhs->upper_ * (int64_t)rhs->lower_;
    int64_t d = (int64_t)lhs->upper_ * (int64_t)rhs->upper_;
    return Range(
        Min( Min(a, b), Min(c, d) ),
        Max( Max(a, b), Max(c, d) ));
}

Range
Range::shl(const Range *lhs, int32 c)
{
    int32 shift = c & 0x1f;
    return Range(
        (int64_t)lhs->lower_ << shift,
        (int64_t)lhs->upper_ << shift);
}

Range
Range::shr(const Range *lhs, int32 c)
{
    int32 shift = c & 0x1f;
    return Range(
        (int64_t)lhs->lower_ >> shift,
        (int64_t)lhs->upper_ >> shift);
}

bool
Range::update(const Range *other)
{
    bool changed =
        lower_ != other->lower_ ||
        lower_infinite_ != other->lower_infinite_ ||
        upper_ != other->upper_ ||
        upper_infinite_ != other->upper_infinite_;

    if (changed) {
        lower_ = other->lower_;
        lower_infinite_ = other->lower_infinite_;
        upper_ = other->upper_;
        upper_infinite_ = other->upper_infinite_;
    }

    return changed;
}

static inline bool
AddToWorklist(MDefinitionVector &worklist, MDefinition *def)
{
    if (def->isInWorklist())
        return true;
    def->setInWorklist();
    return worklist.append(def);
}

static inline MDefinition *
PopFromWorklist(MDefinitionVector &worklist)
{
    JS_ASSERT(!worklist.empty());
    MDefinition *def = worklist.popCopy();
    def->setNotInWorklist();
    return def;
}


bool
RangeAnalysis::analyze()
{
    IonSpew(IonSpew_Range, "Doing range propagation");
    MDefinitionVector worklist;

    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            MDefinition *def = *iter;

            if (!def->isPhi() && !def->isBeta())
                continue;
            AddToWorklist(worklist, def);

        }
    }
    size_t iters = 0;
#define MAX_ITERS 4096
    
    
    
    
    
    while (!worklist.empty() ) {
        MDefinition *def = PopFromWorklist(worklist);
        IonSpew(IonSpew_Range, "recomputing range on %d", def->id());
        SpewRange(def);
        if (def->recomputeRange()) {
            JS_ASSERT(def->range()->lower() <= def->range()->upper());
            IonSpew(IonSpew_Range, "Range changed; adding consumers");
            for (MUseDefIterator use(def); use; use++) {
                if(!AddToWorklist(worklist, use.def()))
                    return false;
            }
        }
        iters++;
    }
    
    for(size_t i = 0; i < worklist.length(); i++)
        worklist[i]->setNotInWorklist();

#undef MAX_ITERS

#ifdef DEBUG
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MDefinitionIterator iter(*block); iter; iter++) {
            MDefinition *def = *iter;
            SpewRange(def);
            JS_ASSERT(def->range()->lower() <= def->range()->upper());
            JS_ASSERT(!def->isInWorklist());
        }
    }
#endif
    return true;
}
