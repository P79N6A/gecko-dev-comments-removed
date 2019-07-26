






#include "mozilla/MathAlgorithms.h"

#include <math.h>
#include <stdio.h>

#include "vm/NumericConversions.h"

#include "Ion.h"
#include "IonAnalysis.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "RangeAnalysis.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

using mozilla::Abs;



























































RangeAnalysis::RangeAnalysis(MIRGraph &graph)
  : graph_(graph)
{
}

static bool
IsDominatedUse(MBasicBlock *block, MUse *use)
{
    MNode *n = use->consumer();
    bool isPhi = n->isDefinition() && n->toDefinition()->isPhi();

    if (isPhi)
        return block->dominates(n->block()->getPredecessor(use->index()));

    return block->dominates(n->block());
}

static inline void
SpewRange(MDefinition *def)
{
#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Range) && def->range()) {
        Sprinter sp(GetIonContext()->cx);
        sp.init();
        def->range()->print(sp);
        IonSpew(IonSpew_Range, "%d has range %s", def->id(), sp.string());
    }
#endif
}

void
RangeAnalysis::replaceDominatedUsesWith(MDefinition *orig, MDefinition *dom,
                                            MBasicBlock *block)
{
    for (MUseIterator i(orig->usesBegin()); i != orig->usesEnd(); ) {
        if (i->consumer() != dom && IsDominatedUse(block, *i))
            i = i->consumer()->replaceOperand(i, dom);
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

        
        if (compare->compareType() == MCompare::Compare_UInt32)
            continue;

        MDefinition *left = compare->getOperand(0);
        MDefinition *right = compare->getOperand(1);
        int32_t bound;
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
            } else if (jsop == JSOP_GT) {
                smaller = right;
                greater = left;
            }
            if (smaller && greater) {
                MBeta *beta;
                beta = MBeta::New(smaller, new Range(JSVAL_INT_MIN, JSVAL_INT_MAX-1,
                                                     smaller->type() != MIRType_Int32,
                                                     Range::MaxDoubleExponent));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(smaller, beta, block);
                IonSpew(IonSpew_Range, "Adding beta node for smaller %d", smaller->id());
                beta = MBeta::New(greater, new Range(JSVAL_INT_MIN+1, JSVAL_INT_MAX,
                                                     greater->type() != MIRType_Int32,
                                                     Range::MaxDoubleExponent));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(greater, beta, block);
                IonSpew(IonSpew_Range, "Adding beta node for greater %d", greater->id());
            }
            continue;
        }

        JS_ASSERT(val);


        Range comp;
        if (val->type() == MIRType_Int32)
            comp.setInt32();
        switch (jsop) {
          case JSOP_LE:
            comp.setUpper(bound);
            break;
          case JSOP_LT:
            if (!SafeSub(bound, 1, &bound))
                break;
            comp.setUpper(bound);
            break;
          case JSOP_GE:
            comp.setLower(bound);
            break;
          case JSOP_GT:
            if (!SafeAdd(bound, 1, &bound))
                break;
            comp.setLower(bound);
            break;
          case JSOP_EQ:
            comp.setLower(bound);
            comp.setUpper(bound);
          default:
            break; 
                   
        }

        IonSpew(IonSpew_Range, "Adding beta node for %d", val->id());
        MBeta *beta = MBeta::New(val, new Range(comp));
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
SymbolicBound::print(Sprinter &sp) const
{
    if (loop)
        sp.printf("[loop] ");
    sum.print(sp);
}

void
Range::print(Sprinter &sp) const
{
    JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
    JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);

    
    if (decimal_)
        sp.printf("R");
    else
        sp.printf("N");

    sp.printf("[");

    if (lower_infinite_)
        sp.printf("-inf");
    else
        sp.printf("%d", lower_);
    if (symbolicLower_) {
        sp.printf(" {");
        symbolicLower_->print(sp);
        sp.printf("}");
    }

    sp.printf(", ");

    if (upper_infinite_)
        sp.printf("inf");
    else
        sp.printf("%d", upper_);
    if (symbolicUpper_) {
        sp.printf(" {");
        symbolicUpper_->print(sp);
        sp.printf("}");
    }

    sp.printf("]");
    sp.printf(" (%db)", numBits());
}

Range *
Range::intersect(const Range *lhs, const Range *rhs, bool *emptyRange)
{
    *emptyRange = false;

    if (!lhs && !rhs)
        return NULL;

    if (!lhs)
        return new Range(*rhs);
    if (!rhs)
        return new Range(*lhs);

    Range *r = new Range(
        Max(lhs->lower_, rhs->lower_),
        Min(lhs->upper_, rhs->upper_),
        lhs->decimal_ && rhs->decimal_,
        Min(lhs->max_exponent_, rhs->max_exponent_));

    r->lower_infinite_ = lhs->lower_infinite_ && rhs->lower_infinite_;
    r->upper_infinite_ = lhs->upper_infinite_ && rhs->upper_infinite_;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (r->upper_ < r->lower_) {
        *emptyRange = true;
        r->makeRangeInfinite();
    }

    return r;
}

void
Range::unionWith(const Range *other)
{
   lower_infinite_ |= other->lower_infinite_;
   upper_infinite_ |= other->upper_infinite_;
   decimal_ |= other->decimal_;
   max_exponent_ = Max(max_exponent_, other->max_exponent_);
   setLower(Min(lower_, other->lower_));
   setUpper(Max(upper_, other->upper_));
}

static const Range emptyRange;

Range::Range(const MDefinition *def)
  : symbolicLower_(NULL),
    symbolicUpper_(NULL)
{
    const Range *other = def->range();
    if (!other)
        other = &emptyRange;

    lower_ = other->lower_;
    lower_infinite_ = other->lower_infinite_;
    upper_ = other->upper_;
    upper_infinite_ = other->upper_infinite_;
    decimal_ = other->decimal_;
    max_exponent_ = other->max_exponent_;

    if (def->type() == MIRType_Int32)
        truncate();
}

const int64_t RANGE_INF_MAX = (int64_t) JSVAL_INT_MAX + 1;
const int64_t RANGE_INF_MIN = (int64_t) JSVAL_INT_MIN - 1;

static inline bool
HasInfinite(const Range *lhs, const Range *rhs)
{
    return lhs->isLowerInfinite() || lhs->isUpperInfinite() ||
           rhs->isLowerInfinite() || rhs->isUpperInfinite();
}

Range *
Range::add(const Range *lhs, const Range *rhs)
{
    int64_t l = (int64_t) lhs->lower_ + (int64_t) rhs->lower_;
    if (lhs->isLowerInfinite() || rhs->isLowerInfinite())
        l = RANGE_INF_MIN;

    int64_t h = (int64_t) lhs->upper_ + (int64_t) rhs->upper_;
    if (lhs->isUpperInfinite() || rhs->isUpperInfinite())
        h = RANGE_INF_MAX;

    return new Range(l, h, lhs->isDecimal() || rhs->isDecimal(),
                     Max(lhs->exponent(), rhs->exponent()) + 1);
}

Range *
Range::sub(const Range *lhs, const Range *rhs)
{
    int64_t l = (int64_t) lhs->lower_ - (int64_t) rhs->upper_;
    if (lhs->isLowerInfinite() || rhs->isUpperInfinite())
        l = RANGE_INF_MIN;

    int64_t h = (int64_t) lhs->upper_ - (int64_t) rhs->lower_;
    if (lhs->isUpperInfinite() || rhs->isLowerInfinite())
        h = RANGE_INF_MAX;

    return new Range(l, h, lhs->isDecimal() || rhs->isDecimal(),
                     Max(lhs->exponent(), rhs->exponent()) + 1);
}

Range *
Range::and_(const Range *lhs, const Range *rhs)
{
    int64_t lower;
    int64_t upper;

    
    if (lhs->lower_ < 0 && rhs->lower_ < 0) {
        lower = INT_MIN;
        upper = Max(lhs->upper_, rhs->upper_);
        return new Range(lower, upper);
    }

    
    
    
    lower = 0;
    upper = Min(lhs->upper_, rhs->upper_);

    
    
    
    if (lhs->lower_ < 0)
       upper = rhs->upper_;
    if (rhs->lower_ < 0)
        upper = lhs->upper_;

    return new Range(lower, upper);
}

Range *
Range::mul(const Range *lhs, const Range *rhs)
{
    bool decimal = lhs->isDecimal() || rhs->isDecimal();
    uint16_t exponent = lhs->numBits() + rhs->numBits() - 1;
    if (HasInfinite(lhs, rhs))
        return new Range(RANGE_INF_MIN, RANGE_INF_MAX, decimal, exponent);
    int64_t a = (int64_t)lhs->lower_ * (int64_t)rhs->lower_;
    int64_t b = (int64_t)lhs->lower_ * (int64_t)rhs->upper_;
    int64_t c = (int64_t)lhs->upper_ * (int64_t)rhs->lower_;
    int64_t d = (int64_t)lhs->upper_ * (int64_t)rhs->upper_;
    return new Range(
        Min( Min(a, b), Min(c, d) ),
        Max( Max(a, b), Max(c, d) ),
        decimal, exponent);
}

Range *
Range::shl(const Range *lhs, int32_t c)
{
    int32_t shift = c & 0x1f;
    return new Range(
        (int64_t)lhs->lower_ << shift,
        (int64_t)lhs->upper_ << shift);
}

Range *
Range::shr(const Range *lhs, int32_t c)
{
    int32_t shift = c & 0x1f;
    return new Range(
        (int64_t)lhs->lower_ >> shift,
        (int64_t)lhs->upper_ >> shift);
}

bool
Range::negativeZeroMul(const Range *lhs, const Range *rhs)
{
    
    if (lhs->lower_ >= 0 && rhs->lower_ >= 0)
        return false;

    
    if (lhs->upper_ < 0 && rhs->upper_ < 0)
        return false;

    
    if (lhs->lower_ > 0 || rhs->lower_ > 0)
        return false;

    return true;
}

bool
Range::update(const Range *other)
{
    bool changed =
        lower_ != other->lower_ ||
        lower_infinite_ != other->lower_infinite_ ||
        upper_ != other->upper_ ||
        upper_infinite_ != other->upper_infinite_ ||
        decimal_ != other->decimal_ ||
        max_exponent_ != other->max_exponent_;
    if (changed) {
        lower_ = other->lower_;
        lower_infinite_ = other->lower_infinite_;
        upper_ = other->upper_;
        upper_infinite_ = other->upper_infinite_;
        decimal_ = other->decimal_;
        max_exponent_ = other->max_exponent_;
    }

    return changed;
}





void
MPhi::computeRange()
{
    if (type() != MIRType_Int32 && type() != MIRType_Double)
        return;

    Range *range = NULL;
    JS_ASSERT(getOperand(0)->op() != MDefinition::Op_OsrValue);
    for (size_t i = 0; i < numOperands(); i++) {
        if (getOperand(i)->block()->earlyAbort()) {
            IonSpew(IonSpew_Range, "Ignoring unreachable input %d", getOperand(i)->id());
            continue;
        }

        if (isOSRLikeValue(getOperand(i)))
            continue;

        Range *input = getOperand(i)->range();

        if (!input) {
            range = NULL;
            break;
        }

        if (range)
            range->unionWith(input);
        else
            range = new Range(*input);
    }

    setRange(range);

    if (block()->isLoopHeader()) {
    }
}

void
MConstant::computeRange()
{
    if (type() == MIRType_Int32) {
        setRange(new Range(value().toInt32(), value().toInt32()));
        return;
    }

    if (type() != MIRType_Double)
        return;

    double d = value().toDouble();
    int exp = Range::MaxDoubleExponent;

    
    if (MOZ_DOUBLE_IS_NaN(d)) {
        setRange(new Range(RANGE_INF_MIN, RANGE_INF_MAX, true, exp));
        return;
    }

    
    if (MOZ_DOUBLE_IS_INFINITE(d)) {
        if (MOZ_DOUBLE_IS_NEGATIVE(d))
            setRange(new Range(RANGE_INF_MIN, RANGE_INF_MIN, false, exp));
        else
            setRange(new Range(RANGE_INF_MAX, RANGE_INF_MAX, false, exp));
        return;
    }

    
    exp = MOZ_DOUBLE_EXPONENT(d);
    if (exp < 0) {
        
        if (MOZ_DOUBLE_IS_NEGATIVE(d))
            setRange(new Range(-1, 0, true, 0));
        else
            setRange(new Range(0, 1, true, 0));
    } else if (exp < Range::MaxTruncatableExponent) {
        
        int64_t integral = ToInt64(d);
        
        double rest = d - (double) integral;
        
        
        int64_t l = integral - ((rest < 0) ? 1 : 0);
        int64_t h = integral + ((rest > 0) ? 1 : 0);
        setRange(new Range(l, h, (rest != 0), exp));
    } else {
        
        
        if (MOZ_DOUBLE_IS_NEGATIVE(d))
            setRange(new Range(RANGE_INF_MIN, RANGE_INF_MIN, false, exp));
        else
            setRange(new Range(RANGE_INF_MAX, RANGE_INF_MAX, false, exp));
    }
}

void
MCharCodeAt::computeRange()
{
    setRange(new Range(0, 65535)); 
                                   
}

void
MClampToUint8::computeRange()
{
    setRange(new Range(0, 255));
}

void
MBitAnd::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    setRange(Range::and_(&left, &right));
}

void
MLsh::computeRange()
{
    MDefinition *right = getOperand(1);
    if (!right->isConstant())
        return;

    int32_t c = right->toConstant()->value().toInt32();
    Range other(getOperand(0));
    setRange(Range::shl(&other, c));
}

void
MRsh::computeRange()
{
    MDefinition *right = getOperand(1);
    if (!right->isConstant())
        return;

    int32_t c = right->toConstant()->value().toInt32();
    Range other(getOperand(0));
    setRange(Range::shr(&other, c));
}

void
MAbs::computeRange()
{
    if (specialization_ != MIRType_Int32 && specialization_ != MIRType_Double)
        return;

    Range other(getOperand(0));

    Range *range = new Range(0,
                             Max(Abs<int64_t>(other.lower()), Abs<int64_t>(other.upper())),
                             other.isDecimal(),
                             other.exponent());
    setRange(range);
}

void
MAdd::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    Range *next = Range::add(&left, &right);
    setRange(next);
}

void
MSub::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    Range *next = Range::sub(&left, &right);
    setRange(next);
}

void
MMul::computeRange()
{
    if ((specialization() != MIRType_Int32 && specialization() != MIRType_Double) || isTruncated())
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    if (canBeNegativeZero())
        canBeNegativeZero_ = Range::negativeZeroMul(&left, &right);
    setRange(Range::mul(&left, &right));
}

void
MMod::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range lhs(getOperand(0));
    Range rhs(getOperand(1));
    int64_t a = Abs<int64_t>(rhs.lower());
    int64_t b = Abs<int64_t>(rhs.upper());
    if (a == 0 && b == 0)
        return;
    int64_t bound = Max(1-a, b-1);
    setRange(new Range(-bound, bound, lhs.isDecimal() || rhs.isDecimal()));
}

void
MToDouble::computeRange()
{
    setRange(new Range(getOperand(0)));
}

void
MTruncateToInt32::computeRange()
{
    Range input(getOperand(0));
    setRange(new Range(input.lower(), input.upper()));
}

void
MToInt32::computeRange()
{
    Range input(getOperand(0));
    setRange(new Range(input.lower(), input.upper()));
}





void
RangeAnalysis::markBlocksInLoopBody(MBasicBlock *header, MBasicBlock *current)
{
    
    current->mark();

    
    
    if (current != header) {
        for (size_t i = 0; i < current->numPredecessors(); i++) {
            if (current->getPredecessor(i)->isMarked())
                continue;
            markBlocksInLoopBody(header, current->getPredecessor(i));
        }
    }
}

void
RangeAnalysis::analyzeLoop(MBasicBlock *header)
{
    
    
    
    MBasicBlock *backedge = header->backedge();

    
    if (backedge == header)
        return;

    markBlocksInLoopBody(header, backedge);

    LoopIterationBound *iterationBound = NULL;

    MBasicBlock *block = backedge;
    do {
        BranchDirection direction;
        MTest *branch = block->immediateDominatorBranch(&direction);

        if (block == block->immediateDominator())
            break;

        block = block->immediateDominator();

        if (branch) {
            direction = NegateBranchDirection(direction);
            MBasicBlock *otherBlock = branch->branchSuccessor(direction);
            if (!otherBlock->isMarked()) {
                iterationBound =
                    analyzeLoopIterationCount(header, branch, direction);
                if (iterationBound)
                    break;
            }
        }
    } while (block != header);

    if (!iterationBound) {
        graph_.unmarkBlocks();
        return;
    }

#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Range)) {
        Sprinter sp(GetIonContext()->cx);
        sp.init();
        iterationBound->sum.print(sp);
        IonSpew(IonSpew_Range, "computed symbolic bound on backedges: %s",
                sp.string());
    }
#endif

    
    

    for (MDefinitionIterator iter(header); iter; iter++) {
        MDefinition *def = *iter;
        if (def->isPhi())
            analyzeLoopPhi(header, iterationBound, def->toPhi());
    }

    

    Vector<MBoundsCheck *, 0, IonAllocPolicy> hoistedChecks;

    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;
        if (!block->isMarked())
            continue;

        for (MDefinitionIterator iter(block); iter; iter++) {
            MDefinition *def = *iter;
            if (def->isBoundsCheck() && def->isMovable()) {
                if (tryHoistBoundsCheck(header, def->toBoundsCheck()))
                    hoistedChecks.append(def->toBoundsCheck());
            }
        }
    }

    
    
    
    
    
    for (size_t i = 0; i < hoistedChecks.length(); i++) {
        MBoundsCheck *ins = hoistedChecks[i];
        ins->replaceAllUsesWith(ins->index());
        ins->block()->discard(ins);
    }

    graph_.unmarkBlocks();
}

LoopIterationBound *
RangeAnalysis::analyzeLoopIterationCount(MBasicBlock *header,
                                         MTest *test, BranchDirection direction)
{
    SimpleLinearSum lhs(NULL, 0);
    MDefinition *rhs;
    bool lessEqual;
    if (!ExtractLinearInequality(test, direction, &lhs, &rhs, &lessEqual))
        return NULL;

    
    if (rhs && rhs->block()->isMarked()) {
        if (lhs.term && lhs.term->block()->isMarked())
            return NULL;
        MDefinition *temp = lhs.term;
        lhs.term = rhs;
        rhs = temp;
        if (!SafeSub(0, lhs.constant, &lhs.constant))
            return NULL;
        lessEqual = !lessEqual;
    }

    JS_ASSERT_IF(rhs, !rhs->block()->isMarked());

    
    if (!lhs.term || !lhs.term->isPhi() || lhs.term->block() != header)
        return NULL;

    
    
    
    

    if (lhs.term->toPhi()->numOperands() != 2)
        return NULL;

    
    
    
    MDefinition *lhsInitial = lhs.term->toPhi()->getOperand(0);
    if (lhsInitial->block()->isMarked())
        return NULL;

    
    
    MDefinition *lhsWrite = lhs.term->toPhi()->getOperand(1);
    if (lhsWrite->isBeta())
        lhsWrite = lhsWrite->getOperand(0);
    if (!lhsWrite->isAdd() && !lhsWrite->isSub())
        return NULL;
    if (!lhsWrite->block()->isMarked())
        return NULL;
    MBasicBlock *bb = header->backedge();
    for (; bb != lhsWrite->block() && bb != header; bb = bb->immediateDominator()) {}
    if (bb != lhsWrite->block())
        return NULL;

    SimpleLinearSum lhsModified = ExtractLinearSum(lhsWrite);

    
    
    
    
    
    
    
    if (lhsModified.term != lhs.term)
        return NULL;

    LinearSum bound;

    if (lhsModified.constant == 1 && !lessEqual) {
        
        
        
        
        
        

        if (rhs) {
            if (!bound.add(rhs, 1))
                return NULL;
        }
        if (!bound.add(lhsInitial, -1))
            return NULL;

        int32_t lhsConstant;
        if (!SafeSub(0, lhs.constant, &lhsConstant))
            return NULL;
        if (!bound.add(lhsConstant))
            return NULL;
    } else if (lhsModified.constant == -1 && lessEqual) {
        
        
        
        
        

        if (!bound.add(lhsInitial, 1))
            return NULL;
        if (rhs) {
            if (!bound.add(rhs, -1))
                return NULL;
        }
        if (!bound.add(lhs.constant))
            return NULL;
    } else {
        return NULL;
    }

    return new LoopIterationBound(header, test, bound);
}

void
RangeAnalysis::analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi)
{
    
    
    
    
    
    

    if (phi->numOperands() != 2)
        return;

    MBasicBlock *preLoop = header->loopPredecessor();
    JS_ASSERT(!preLoop->isMarked() && preLoop->successorWithPhis() == header);

    MBasicBlock *backedge = header->backedge();
    JS_ASSERT(backedge->isMarked() && backedge->successorWithPhis() == header);

    MDefinition *initial = phi->getOperand(preLoop->positionInPhiSuccessor());
    if (initial->block()->isMarked())
        return;

    SimpleLinearSum modified = ExtractLinearSum(phi->getOperand(backedge->positionInPhiSuccessor()));

    if (modified.term != phi || modified.constant == 0)
        return;

    if (!phi->range())
        phi->setRange(new Range());

    LinearSum initialSum;
    if (!initialSum.add(initial, 1))
        return;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    LinearSum limitSum(loopBound->sum);
    if (!limitSum.multiply(modified.constant) || !limitSum.add(initialSum))
        return;

    int32_t negativeConstant;
    if (!SafeSub(0, modified.constant, &negativeConstant) || !limitSum.add(negativeConstant))
        return;

    if (modified.constant > 0) {
        phi->range()->setSymbolicLower(new SymbolicBound(NULL, initialSum));
        phi->range()->setSymbolicUpper(new SymbolicBound(loopBound, limitSum));
    } else {
        phi->range()->setSymbolicUpper(new SymbolicBound(NULL, initialSum));
        phi->range()->setSymbolicLower(new SymbolicBound(loopBound, limitSum));
    }

    IonSpew(IonSpew_Range, "added symbolic range on %d", phi->id());
    SpewRange(phi);
}



static inline bool
SymbolicBoundIsValid(MBasicBlock *header, MBoundsCheck *ins, const SymbolicBound *bound)
{
    if (!bound->loop)
        return true;
    if (ins->block() == header)
        return false;
    MBasicBlock *bb = ins->block()->immediateDominator();
    while (bb != header && bb != bound->loop->test->block())
        bb = bb->immediateDominator();
    return bb == bound->loop->test->block();
}



static inline MDefinition *
ConvertLinearSum(MBasicBlock *block, const LinearSum &sum)
{
    MDefinition *def = NULL;

    for (size_t i = 0; i < sum.numTerms(); i++) {
        LinearTerm term = sum.term(i);
        JS_ASSERT(!term.term->isConstant());
        if (term.scale == 1) {
            if (def) {
                def = MAdd::New(def, term.term);
                def->toAdd()->setInt32();
                block->insertBefore(block->lastIns(), def->toInstruction());
            } else {
                def = term.term;
            }
        } else {
            if (!def) {
                def = MConstant::New(Int32Value(0));
                block->insertBefore(block->lastIns(), def->toInstruction());
            }
            if (term.scale == -1) {
                def = MSub::New(def, term.term);
                def->toSub()->setInt32();
                block->insertBefore(block->lastIns(), def->toInstruction());
            } else {
                MConstant *factor = MConstant::New(Int32Value(term.scale));
                block->insertBefore(block->lastIns(), factor);
                MMul *mul = MMul::New(term.term, factor);
                mul->setInt32();
                block->insertBefore(block->lastIns(), mul);
                def = MAdd::New(def, mul);
                def->toAdd()->setInt32();
                block->insertBefore(block->lastIns(), def->toInstruction());
            }
        }
    }

    if (!def) {
        def = MConstant::New(Int32Value(0));
        block->insertBefore(block->lastIns(), def->toInstruction());
    }

    return def;
}

bool
RangeAnalysis::tryHoistBoundsCheck(MBasicBlock *header, MBoundsCheck *ins)
{
    
    if (ins->length()->block()->isMarked())
        return false;

    
    
    SimpleLinearSum index = ExtractLinearSum(ins->index());
    if (!index.term || !index.term->block()->isMarked())
        return false;

    
    
    
    if (!index.term->range())
        return false;
    const SymbolicBound *lower = index.term->range()->symbolicLower();
    if (!lower || !SymbolicBoundIsValid(header, ins, lower))
        return false;
    const SymbolicBound *upper = index.term->range()->symbolicUpper();
    if (!upper || !SymbolicBoundIsValid(header, ins, upper))
        return false;

    MBasicBlock *preLoop = header->loopPredecessor();
    JS_ASSERT(!preLoop->isMarked());

    MDefinition *lowerTerm = ConvertLinearSum(preLoop, lower->sum);
    if (!lowerTerm)
        return false;

    MDefinition *upperTerm = ConvertLinearSum(preLoop, upper->sum);
    if (!upperTerm)
        return false;

    
    
    
    
    

    int32_t lowerConstant = 0;
    if (!SafeSub(lowerConstant, index.constant, &lowerConstant))
        return false;
    if (!SafeSub(lowerConstant, lower->sum.constant(), &lowerConstant))
        return false;
    MBoundsCheckLower *lowerCheck = MBoundsCheckLower::New(lowerTerm);
    lowerCheck->setMinimum(lowerConstant);

    
    
    
    

    int32_t upperConstant = index.constant;
    if (!SafeAdd(upper->sum.constant(), upperConstant, &upperConstant))
        return false;
    MBoundsCheck *upperCheck = MBoundsCheck::New(upperTerm, ins->length());
    upperCheck->setMinimum(upperConstant);
    upperCheck->setMaximum(upperConstant);

    
    preLoop->insertBefore(preLoop->lastIns(), lowerCheck);
    preLoop->insertBefore(preLoop->lastIns(), upperCheck);

    return true;
}

bool
RangeAnalysis::analyze()
{
    IonSpew(IonSpew_Range, "Doing range propagation");

    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;

        for (MDefinitionIterator iter(block); iter; iter++) {
            MDefinition *def = *iter;

            def->computeRange();
            IonSpew(IonSpew_Range, "computing range on %d", def->id());
            SpewRange(def);
        }

        if (block->isLoopHeader())
            analyzeLoop(block);
    }

    return true;
}





void
Range::truncate()
{
    if (isInt32())
        return;
    int64_t l = isLowerInfinite() ? JSVAL_INT_MIN : lower();
    int64_t h = isUpperInfinite() ? JSVAL_INT_MAX : upper();
    set(l, h, false, 32);
}

bool
MDefinition::truncate()
{
    
    return false;
}

bool
MConstant::truncate()
{
    if (!value_.isDouble())
        return false;

    
    value_.setInt32(ToInt32(value_.toDouble()));
    setResultType(MIRType_Int32);
    if (range())
        range()->truncate();
    return true;
}

bool
MAdd::truncate()
{
    
    setTruncated(true);

    
    if (type() != MIRType_Double)
        return false;

    specialization_ = MIRType_Int32;
    setResultType(MIRType_Int32);
    if (range())
        range()->truncate();
    return true;
}

bool
MSub::truncate()
{
    
    setTruncated(true);

    
    if (type() != MIRType_Double)
        return false;

    specialization_ = MIRType_Int32;
    setResultType(MIRType_Int32);
    if (range())
        range()->truncate();
    return true;
}

bool
MMul::truncate()
{
    
    setTruncated(true);

    
    bool truncated = type() == MIRType_Int32;
    if (type() == MIRType_Double) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        truncated = true;
        JS_ASSERT(range());
    }

    if (truncated && range()) {
        range()->truncate();
        setTruncated(true);
        setCanBeNegativeZero(false);
    }

    return truncated;
}

bool
MDiv::truncate()
{
    
    setTruncated(true);

    
    return false;
}

bool
MMod::truncate()
{
    
    setTruncated(true);

    
    return false;
}

bool
MToDouble::truncate()
{
    JS_ASSERT(type() == MIRType_Double);

    
    
    setResultType(MIRType_Int32);
    if (range())
        range()->truncate();

    return true;
}

bool
MDefinition::isOperandTruncated(size_t index) const
{
    return false;
}

bool
MTruncateToInt32::isOperandTruncated(size_t index) const
{
    return true;
}

bool
MBinaryBitwiseInstruction::isOperandTruncated(size_t index) const
{
    return true;
}

bool
MAdd::isOperandTruncated(size_t index) const
{
    return isTruncated();
}

bool
MSub::isOperandTruncated(size_t index) const
{
    return isTruncated();
}

bool
MMul::isOperandTruncated(size_t index) const
{
    return isTruncated();
}

bool
MToDouble::isOperandTruncated(size_t index) const
{
    
    
    return type() == MIRType_Int32;
}



static bool
AllUsesTruncate(MInstruction *candidate)
{
    for (MUseDefIterator use(candidate); use; use++) {
        if (!use.def()->isOperandTruncated(use.index()))
            return false;
    }

    return true;
}

static void
RemoveTruncatesOnOutput(MInstruction *truncated)
{
    JS_ASSERT(truncated->type() == MIRType_Int32);
    JS_ASSERT_IF(truncated->range(), truncated->range()->isInt32());

    for (MUseDefIterator use(truncated); use; use++) {
        MDefinition *def = use.def();
        if (!def->isTruncateToInt32() || !def->isToInt32())
            continue;

        def->replaceAllUsesWith(truncated);
    }
}

void
AdjustTruncatedInputs(MInstruction *truncated)
{
    MBasicBlock *block = truncated->block();
    for (size_t i = 0; i < truncated->numOperands(); i++) {
        if (!truncated->isOperandTruncated(i))
            continue;
        if (truncated->getOperand(i)->type() == MIRType_Int32)
            continue;

        MTruncateToInt32 *op = MTruncateToInt32::New(truncated->getOperand(i));
        block->insertBefore(truncated, op);
        truncated->replaceOperand(i, op);
    }

    if (truncated->isToDouble()) {
        truncated->replaceAllUsesWith(truncated->getOperand(0));
        block->discard(truncated);
    }
}












bool
RangeAnalysis::truncate()
{
    IonSpew(IonSpew_Range, "Do range-base truncation (backward loop)");

    Vector<MInstruction *, 16, SystemAllocPolicy> worklist;

    for (PostorderIterator block(graph_.poBegin()); block != graph_.poEnd(); block++) {
        for (MInstructionReverseIterator iter(block->rbegin()); iter != block->rend(); iter++) {
            
            
            const Range *r = iter->range();
            if (!r || r->hasRoundingErrors())
                continue;

            
            if (!AllUsesTruncate(*iter))
                continue;

            
            if (!iter->truncate())
                continue;

            
            
            iter->setInWorklist();
            if (!worklist.append(*iter))
                return false;
        }
    }

    
    IonSpew(IonSpew_Range, "Do graph type fixup (dequeue)");
    while (!worklist.empty()) {
        MInstruction *ins = worklist.popCopy();
        ins->setNotInWorklist();
        RemoveTruncatesOnOutput(ins);
        AdjustTruncatedInputs(ins);
    }

    return true;
}
