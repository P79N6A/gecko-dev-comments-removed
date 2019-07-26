





#include "jit/RangeAnalysis.h"

#include "mozilla/MathAlgorithms.h"

#include "jsanalyze.h"

#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "vm/NumericConversions.h"

using namespace js;
using namespace js::jit;

using mozilla::Abs;
using mozilla::CountLeadingZeroes32;
using mozilla::DoubleIsInt32;
using mozilla::ExponentComponent;
using mozilla::IsInfinite;
using mozilla::IsFinite;
using mozilla::IsNaN;
using mozilla::IsNegative;
using mozilla::Swap;



























































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
    if (IonSpewEnabled(IonSpew_Range) && def->type() != MIRType_None && def->range()) {
        IonSpewHeader(IonSpew_Range);
        def->printName(IonSpewFile);
        fprintf(IonSpewFile, " has range ");
        def->range()->dump(IonSpewFile);
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
RangeAnalysis::addBetaNodes()
{
    IonSpew(IonSpew_Range, "Adding beta nodes");

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
        double bound;
        int16_t exponent = Range::IncludesInfinity;
        MDefinition *val = nullptr;

        JSOp jsop = compare->jsop();

        if (branch_dir == FALSE_BRANCH) {
            jsop = analyze::NegateCompareOp(jsop);
            exponent = Range::IncludesInfinityAndNaN;
        }

        if (left->isConstant() && left->toConstant()->value().isNumber()) {
            bound = left->toConstant()->value().toNumber();
            val = right;
            jsop = analyze::ReverseCompareOp(jsop);
        } else if (right->isConstant() && right->toConstant()->value().isNumber()) {
            bound = right->toConstant()->value().toNumber();
            val = left;
        } else if (left->type() == MIRType_Int32 && right->type() == MIRType_Int32) {
            MDefinition *smaller = nullptr;
            MDefinition *greater = nullptr;
            if (jsop == JSOP_LT) {
                smaller = left;
                greater = right;
            } else if (jsop == JSOP_GT) {
                smaller = right;
                greater = left;
            }
            if (smaller && greater) {
                MBeta *beta;
                beta = MBeta::New(smaller, Range::NewInt32Range(JSVAL_INT_MIN, JSVAL_INT_MAX-1));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(smaller, beta, block);
                IonSpew(IonSpew_Range, "Adding beta node for smaller %d", smaller->id());
                beta = MBeta::New(greater, Range::NewInt32Range(JSVAL_INT_MIN+1, JSVAL_INT_MAX));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(greater, beta, block);
                IonSpew(IonSpew_Range, "Adding beta node for greater %d", greater->id());
            }
            continue;
        } else {
            continue;
        }

        
        
        JS_ASSERT(val);

        
        
        if (!IsFinite(bound) || bound < INT32_MIN || bound > INT32_MAX)
            continue;

        Range comp;
        switch (jsop) {
          case JSOP_LE:
            comp.set(Range::NoInt32LowerBound, ceil(bound), true, exponent);
            break;
          case JSOP_LT:
            
            if (val->type() == MIRType_Int32) {
                int32_t intbound;
                if (DoubleIsInt32(bound, &intbound) && SafeSub(intbound, 1, &intbound))
                    bound = intbound;
            }
            comp.set(Range::NoInt32LowerBound, ceil(bound), true, exponent);
            break;
          case JSOP_GE:
            comp.set(floor(bound), Range::NoInt32UpperBound, true, exponent);
            break;
          case JSOP_GT:
            
            if (val->type() == MIRType_Int32) {
                int32_t intbound;
                if (DoubleIsInt32(bound, &intbound) && SafeAdd(intbound, 1, &intbound))
                    bound = intbound;
            }
            comp.set(floor(bound), Range::NoInt32UpperBound, true, exponent);
            break;
          case JSOP_EQ:
            comp.set(floor(bound), ceil(bound), true, exponent);
            break;
          default:
            continue; 
                      
        }

        if (IonSpewEnabled(IonSpew_Range)) {
            IonSpewHeader(IonSpew_Range);
            fprintf(IonSpewFile, "Adding beta node for %d with range ", val->id());
            comp.dump(IonSpewFile);
        }

        MBeta *beta = MBeta::New(val, new Range(comp));
        block->insertBefore(*block->begin(), beta);
        replaceDominatedUsesWith(val, beta, block);
    }

    return true;
}

bool
RangeAnalysis::removeBetaNodes()
{
    IonSpew(IonSpew_Range, "Removing beta nodes");

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
    assertInvariants();

    
    if (canHaveFractionalPart_)
        sp.printf("F");
    else
        sp.printf("I");

    sp.printf("[");

    if (!hasInt32LowerBound_)
        sp.printf("?");
    else
        sp.printf("%d", lower_);
    if (symbolicLower_) {
        sp.printf(" {");
        symbolicLower_->print(sp);
        sp.printf("}");
    }

    sp.printf(", ");

    if (!hasInt32UpperBound_)
        sp.printf("?");
    else
        sp.printf("%d", upper_);
    if (symbolicUpper_) {
        sp.printf(" {");
        symbolicUpper_->print(sp);
        sp.printf("}");
    }

    sp.printf("]");
    if (max_exponent_ == IncludesInfinityAndNaN)
        sp.printf(" (U inf U NaN)", max_exponent_);
    else if (max_exponent_ == IncludesInfinity)
        sp.printf(" (U inf)");
    else if (!hasInt32UpperBound_ || !hasInt32LowerBound_)
        sp.printf(" (< pow(2, %d+1))", max_exponent_);
}

void
Range::dump(FILE *fp) const
{
    Sprinter sp(GetIonContext()->cx);
    sp.init();
    print(sp);
    fprintf(fp, "%s\n", sp.string());
}

Range *
Range::intersect(const Range *lhs, const Range *rhs, bool *emptyRange)
{
    *emptyRange = false;

    if (!lhs && !rhs)
        return nullptr;

    if (!lhs)
        return new Range(*rhs);
    if (!rhs)
        return new Range(*lhs);

    int32_t newLower = Max(lhs->lower_, rhs->lower_);
    int32_t newUpper = Min(lhs->upper_, rhs->upper_);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (newUpper < newLower) {
        *emptyRange = true;
        return nullptr;
    }

    bool newHasInt32LowerBound = lhs->hasInt32LowerBound_ || rhs->hasInt32LowerBound_;
    bool newHasInt32UpperBound = lhs->hasInt32UpperBound_ || rhs->hasInt32UpperBound_;
    bool newFractional = lhs->canHaveFractionalPart_ && rhs->canHaveFractionalPart_;
    uint16_t newExponent = Min(lhs->max_exponent_, rhs->max_exponent_);

    return new Range(newLower, newHasInt32LowerBound, newUpper, newHasInt32UpperBound,
                     newFractional, newExponent);
}

void
Range::unionWith(const Range *other)
{
    int32_t newLower = Min(lower_, other->lower_);
    int32_t newUpper = Max(upper_, other->upper_);

    bool newHasInt32LowerBound = hasInt32LowerBound_ && other->hasInt32LowerBound_;
    bool newHasInt32UpperBound = hasInt32UpperBound_ && other->hasInt32UpperBound_;
    bool newFractional = canHaveFractionalPart_ || other->canHaveFractionalPart_;
    uint16_t newExponent = Max(max_exponent_, other->max_exponent_);

    rawInitialize(newLower, newHasInt32LowerBound, newUpper, newHasInt32UpperBound,
                  newFractional, newExponent);
}

Range::Range(const MDefinition *def)
  : symbolicLower_(nullptr),
    symbolicUpper_(nullptr)
{
    const Range *other = def->range();
    if (!other) {
        if (def->type() == MIRType_Int32)
            setInt32(JSVAL_INT_MIN, JSVAL_INT_MAX);
        else if (def->type() == MIRType_Boolean)
            setInt32(0, 1);
        else
            setUnknown();
        symbolicLower_ = symbolicUpper_ = nullptr;
        return;
    }

    JS_ASSERT_IF(def->type() == MIRType_Boolean, other->isBoolean());

    *this = *other;
    symbolicLower_ = symbolicUpper_ = nullptr;
}

static inline bool
MissingAnyInt32Bounds(const Range *lhs, const Range *rhs)
{
    return !lhs->hasInt32LowerBound() || !lhs->hasInt32UpperBound() ||
           !rhs->hasInt32LowerBound() || !rhs->hasInt32UpperBound();
}

Range *
Range::add(const Range *lhs, const Range *rhs)
{
    int64_t l = (int64_t) lhs->lower_ + (int64_t) rhs->lower_;
    if (!lhs->hasInt32LowerBound() || !rhs->hasInt32LowerBound())
        l = NoInt32LowerBound;

    int64_t h = (int64_t) lhs->upper_ + (int64_t) rhs->upper_;
    if (!lhs->hasInt32UpperBound() || !rhs->hasInt32UpperBound())
        h = NoInt32UpperBound;

    
    
    uint16_t e = Max(lhs->max_exponent_, rhs->max_exponent_);
    if (e <= Range::MaxFiniteExponent)
        ++e;

    
    if (lhs->canBeInfiniteOrNaN() && rhs->canBeInfiniteOrNaN())
        e = Range::IncludesInfinityAndNaN;

    return new Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(), e);
}

Range *
Range::sub(const Range *lhs, const Range *rhs)
{
    int64_t l = (int64_t) lhs->lower_ - (int64_t) rhs->upper_;
    if (!lhs->hasInt32LowerBound() || !rhs->hasInt32UpperBound())
        l = NoInt32LowerBound;

    int64_t h = (int64_t) lhs->upper_ - (int64_t) rhs->lower_;
    if (!lhs->hasInt32UpperBound() || !rhs->hasInt32LowerBound())
        h = NoInt32UpperBound;

    
    
    uint16_t e = Max(lhs->max_exponent_, rhs->max_exponent_);
    if (e <= Range::MaxFiniteExponent)
        ++e;

    
    if (lhs->canBeInfiniteOrNaN() && rhs->canBeInfiniteOrNaN())
        e = Range::IncludesInfinityAndNaN;

    return new Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(), e);
}

Range *
Range::and_(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());

    
    if (lhs->lower() < 0 && rhs->lower() < 0)
        return Range::NewInt32Range(INT32_MIN, Max(lhs->upper(), rhs->upper()));

    
    
    
    int32_t lower = 0;
    int32_t upper = Min(lhs->upper(), rhs->upper());

    
    
    
    if (lhs->lower() < 0)
       upper = rhs->upper();
    if (rhs->lower() < 0)
        upper = lhs->upper();

    return Range::NewInt32Range(lower, upper);
}

Range *
Range::or_(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    
    
    
    
    if (lhs->lower() == lhs->upper()) {
        if (lhs->lower() == 0)
            return new Range(*rhs);
        if (lhs->lower() == -1)
            return new Range(*lhs);;
    }
    if (rhs->lower() == rhs->upper()) {
        if (rhs->lower() == 0)
            return new Range(*lhs);
        if (rhs->lower() == -1)
            return new Range(*rhs);;
    }

    
    
    JS_ASSERT_IF(lhs->lower() >= 0, lhs->upper() != 0);
    JS_ASSERT_IF(rhs->lower() >= 0, rhs->upper() != 0);
    JS_ASSERT_IF(lhs->upper() < 0, lhs->lower() != -1);
    JS_ASSERT_IF(rhs->upper() < 0, rhs->lower() != -1);

    int32_t lower = INT32_MIN;
    int32_t upper = INT32_MAX;

    if (lhs->lower() >= 0 && rhs->lower() >= 0) {
        
        lower = Max(lhs->lower(), rhs->lower());
        
        
        
        upper = int32_t(UINT32_MAX >> Min(CountLeadingZeroes32(lhs->upper()),
                                          CountLeadingZeroes32(rhs->upper())));
    } else {
        
        if (lhs->upper() < 0) {
            unsigned leadingOnes = CountLeadingZeroes32(~lhs->lower());
            lower = Max(lower, ~int32_t(UINT32_MAX >> leadingOnes));
            upper = -1;
        }
        if (rhs->upper() < 0) {
            unsigned leadingOnes = CountLeadingZeroes32(~rhs->lower());
            lower = Max(lower, ~int32_t(UINT32_MAX >> leadingOnes));
            upper = -1;
        }
    }

    return Range::NewInt32Range(lower, upper);
}

Range *
Range::xor_(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    int32_t lhsLower = lhs->lower();
    int32_t lhsUpper = lhs->upper();
    int32_t rhsLower = rhs->lower();
    int32_t rhsUpper = rhs->upper();
    bool invertAfter = false;

    
    
    
    
    if (lhsUpper < 0) {
        lhsLower = ~lhsLower;
        lhsUpper = ~lhsUpper;
        Swap(lhsLower, lhsUpper);
        invertAfter = !invertAfter;
    }
    if (rhsUpper < 0) {
        rhsLower = ~rhsLower;
        rhsUpper = ~rhsUpper;
        Swap(rhsLower, rhsUpper);
        invertAfter = !invertAfter;
    }

    
    
    
    
    int32_t lower = INT32_MIN;
    int32_t upper = INT32_MAX;
    if (lhsLower == 0 && lhsUpper == 0) {
        upper = rhsUpper;
        lower = rhsLower;
    } else if (rhsLower == 0 && rhsUpper == 0) {
        upper = lhsUpper;
        lower = lhsLower;
    } else if (lhsLower >= 0 && rhsLower >= 0) {
        
        lower = 0;
        
        
        
        
        unsigned lhsLeadingZeros = CountLeadingZeroes32(lhsUpper);
        unsigned rhsLeadingZeros = CountLeadingZeroes32(rhsUpper);
        upper = Min(rhsUpper | int32_t(UINT32_MAX >> lhsLeadingZeros),
                    lhsUpper | int32_t(UINT32_MAX >> rhsLeadingZeros));
    }

    
    
    if (invertAfter) {
        lower = ~lower;
        upper = ~upper;
        Swap(lower, upper);
    }

    return Range::NewInt32Range(lower, upper);
}

Range *
Range::not_(const Range *op)
{
    JS_ASSERT(op->isInt32());
    return Range::NewInt32Range(~op->upper(), ~op->lower());
}

Range *
Range::mul(const Range *lhs, const Range *rhs)
{
    bool fractional = lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart();

    uint16_t exponent;
    if (!lhs->canBeInfiniteOrNaN() && !rhs->canBeInfiniteOrNaN()) {
        
        exponent = lhs->numBits() + rhs->numBits() - 1;
        if (exponent > Range::MaxFiniteExponent)
            exponent = Range::IncludesInfinity;
    } else if (!lhs->canBeNaN() &&
               !rhs->canBeNaN() &&
               !(lhs->canBeZero() && rhs->canBeInfiniteOrNaN()) &&
               !(rhs->canBeZero() && lhs->canBeInfiniteOrNaN()))
    {
        
        exponent = Range::IncludesInfinity;
    } else {
        
        exponent = Range::IncludesInfinityAndNaN;
    }

    if (MissingAnyInt32Bounds(lhs, rhs))
        return new Range(NoInt32LowerBound, NoInt32UpperBound, fractional, exponent);
    int64_t a = (int64_t)lhs->lower() * (int64_t)rhs->lower();
    int64_t b = (int64_t)lhs->lower() * (int64_t)rhs->upper();
    int64_t c = (int64_t)lhs->upper() * (int64_t)rhs->lower();
    int64_t d = (int64_t)lhs->upper() * (int64_t)rhs->upper();
    return new Range(
        Min( Min(a, b), Min(c, d) ),
        Max( Max(a, b), Max(c, d) ),
        fractional, exponent);
}

Range *
Range::lsh(const Range *lhs, int32_t c)
{
    JS_ASSERT(lhs->isInt32());
    int32_t shift = c & 0x1f;

    
    
    if ((int32_t)((uint32_t)lhs->lower() << shift << 1 >> shift >> 1) == lhs->lower() &&
        (int32_t)((uint32_t)lhs->upper() << shift << 1 >> shift >> 1) == lhs->upper())
    {
        return Range::NewInt32Range(
            uint32_t(lhs->lower()) << shift,
            uint32_t(lhs->upper()) << shift);
    }

    return Range::NewInt32Range(INT32_MIN, INT32_MAX);
}

Range *
Range::rsh(const Range *lhs, int32_t c)
{
    JS_ASSERT(lhs->isInt32());
    int32_t shift = c & 0x1f;
    return Range::NewInt32Range(
        lhs->lower() >> shift,
        lhs->upper() >> shift);
}

Range *
Range::ursh(const Range *lhs, int32_t c)
{
    
    
    
    JS_ASSERT(lhs->isInt32());

    int32_t shift = c & 0x1f;

    
    
    if (lhs->isFiniteNonNegative() || lhs->isFiniteNegative()) {
        return Range::NewUInt32Range(
            uint32_t(lhs->lower()) >> shift,
            uint32_t(lhs->upper()) >> shift);
    }

    
    return Range::NewUInt32Range(0, UINT32_MAX >> shift);
}

Range *
Range::lsh(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewInt32Range(INT32_MIN, INT32_MAX);
}

Range *
Range::rsh(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewInt32Range(Min(lhs->lower(), 0), Max(lhs->upper(), 0));
}

Range *
Range::ursh(const Range *lhs, const Range *rhs)
{
    
    
    
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewUInt32Range(0, lhs->isFiniteNonNegative() ? lhs->upper() : UINT32_MAX);
}

Range *
Range::abs(const Range *op)
{
    int32_t l = op->lower_;
    int32_t u = op->upper_;

    return new Range(Max(Max(int32_t(0), l), u == INT32_MIN ? INT32_MAX : -u),
                     true,
                     Max(Max(int32_t(0), u), l == INT32_MIN ? INT32_MAX : -l),
                     op->hasInt32LowerBound_ && op->hasInt32UpperBound_ && l != INT32_MIN,
                     op->canHaveFractionalPart_,
                     op->max_exponent_);
}

Range *
Range::min(const Range *lhs, const Range *rhs)
{
    
    if (lhs->canBeNaN() || rhs->canBeNaN())
        return nullptr;

    return new Range(Min(lhs->lower_, rhs->lower_),
                     lhs->hasInt32LowerBound_ && rhs->hasInt32LowerBound_,
                     Min(lhs->upper_, rhs->upper_),
                     lhs->hasInt32UpperBound_ || rhs->hasInt32UpperBound_,
                     lhs->canHaveFractionalPart_ || rhs->canHaveFractionalPart_,
                     Max(lhs->max_exponent_, rhs->max_exponent_));
}

Range *
Range::max(const Range *lhs, const Range *rhs)
{
    
    if (lhs->canBeNaN() || rhs->canBeNaN())
        return nullptr;

    return new Range(Max(lhs->lower_, rhs->lower_),
                     lhs->hasInt32LowerBound_ || rhs->hasInt32LowerBound_,
                     Max(lhs->upper_, rhs->upper_),
                     lhs->hasInt32UpperBound_ && rhs->hasInt32UpperBound_,
                     lhs->canHaveFractionalPart_ || rhs->canHaveFractionalPart_,
                     Max(lhs->max_exponent_, rhs->max_exponent_));
}

bool
Range::negativeZeroMul(const Range *lhs, const Range *rhs)
{
    
    
    return (lhs->canBeFiniteNegative() && rhs->canBeFiniteNonNegative()) ||
           (rhs->canBeFiniteNegative() && lhs->canBeFiniteNonNegative());
}

bool
Range::update(const Range *other)
{
    bool changed =
        lower_ != other->lower_ ||
        hasInt32LowerBound_ != other->hasInt32LowerBound_ ||
        upper_ != other->upper_ ||
        hasInt32UpperBound_ != other->hasInt32UpperBound_ ||
        canHaveFractionalPart_ != other->canHaveFractionalPart_ ||
        max_exponent_ != other->max_exponent_;
    if (changed) {
        lower_ = other->lower_;
        hasInt32LowerBound_ = other->hasInt32LowerBound_;
        upper_ = other->upper_;
        hasInt32UpperBound_ = other->hasInt32UpperBound_;
        canHaveFractionalPart_ = other->canHaveFractionalPart_;
        max_exponent_ = other->max_exponent_;
        assertInvariants();
    }

    return changed;
}





void
MPhi::computeRange()
{
    if (type() != MIRType_Int32 && type() != MIRType_Double)
        return;

    Range *range = nullptr;
    JS_ASSERT(getOperand(0)->op() != MDefinition::Op_OsrValue);
    for (size_t i = 0, e = numOperands(); i < e; i++) {
        if (getOperand(i)->block()->earlyAbort()) {
            IonSpew(IonSpew_Range, "Ignoring unreachable input %d", getOperand(i)->id());
            continue;
        }

        if (isOSRLikeValue(getOperand(i)))
            continue;

        
        
        if (!getOperand(i)->range())
            return;

        Range input(getOperand(i));

        if (range)
            range->unionWith(&input);
        else
            range = new Range(input);
    }

    setRange(range);
}

void
MBeta::computeRange()
{
    bool emptyRange = false;

    Range opRange(getOperand(0));
    Range *range = Range::intersect(&opRange, comparison_, &emptyRange);
    if (emptyRange) {
        IonSpew(IonSpew_Range, "Marking block for inst %d unexitable", id());
        block()->setEarlyAbort();
    } else {
        setRange(range);
    }
}

void
MConstant::computeRange()
{
    if (type() == MIRType_Int32) {
        setRange(Range::NewSingleValueRange(value().toInt32()));
        return;
    }

    if (type() != MIRType_Double)
        return;

    double d = value().toDouble();

    
    if (IsNaN(d))
        return;

    
    if (IsInfinite(d)) {
        if (IsNegative(d))
            setRange(Range::NewDoubleRange(Range::NoInt32LowerBound,
                                           Range::NoInt32LowerBound,
                                           Range::IncludesInfinity));
        else
            setRange(Range::NewDoubleRange(Range::NoInt32UpperBound,
                                           Range::NoInt32UpperBound,
                                           Range::IncludesInfinity));
        return;
    }

    
    int exp = ExponentComponent(d);
    if (exp < 0) {
        
        if (IsNegative(d))
            setRange(Range::NewDoubleRange(-1, 0));
        else
            setRange(Range::NewDoubleRange(0, 1));
    } else if (exp < Range::MaxTruncatableExponent) {
        
        int64_t integral = ToInt64(d);
        
        double rest = d - (double) integral;
        
        
        int64_t l = integral - ((rest < 0) ? 1 : 0);
        int64_t h = integral + ((rest > 0) ? 1 : 0);
        
        if ((rest < 0 && (l == INT64_MIN || IsPowerOfTwo(Abs(l)))) ||
            (rest > 0 && (h == INT64_MIN || IsPowerOfTwo(Abs(h)))))
        {
            ++exp;
        }
        setRange(new Range(l, h, (rest != 0), exp));
    } else {
        
        
        if (IsNegative(d))
            setRange(Range::NewDoubleRange(Range::NoInt32LowerBound,
                                           Range::NoInt32LowerBound,
                                           exp));
        else
            setRange(Range::NewDoubleRange(Range::NoInt32UpperBound,
                                           Range::NoInt32UpperBound,
                                           exp));
    }
}

void
MCharCodeAt::computeRange()
{
    
    setRange(Range::NewInt32Range(0, 65535));
}

void
MClampToUint8::computeRange()
{
    setRange(Range::NewUInt32Range(0, 255));
}

void
MBitAnd::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::and_(&left, &right));
}

void
MBitOr::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::or_(&left, &right));
}

void
MBitXor::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::xor_(&left, &right));
}

void
MBitNot::computeRange()
{
    Range op(getOperand(0));
    op.wrapAroundToInt32();

    setRange(Range::not_(&op));
}

void
MLsh::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        right.wrapAroundToShiftCount();
        setRange(Range::lsh(&left, &right));
        return;
    }

    int32_t c = rhs->toConstant()->value().toInt32();
    setRange(Range::lsh(&left, c));
}

void
MRsh::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        right.wrapAroundToShiftCount();
        setRange(Range::rsh(&left, &right));
        return;
    }

    int32_t c = rhs->toConstant()->value().toInt32();
    setRange(Range::rsh(&left, c));
}

void
MUrsh::computeRange()
{
    Range left(getOperand(0));
    Range right(getOperand(1));

    
    
    
    
    
    left.wrapAroundToInt32();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        right.wrapAroundToShiftCount();
        setRange(Range::ursh(&left, &right));
    } else {
        int32_t c = rhs->toConstant()->value().toInt32();
        setRange(Range::ursh(&left, c));
    }

    JS_ASSERT(range()->lower() >= 0);
    if (type() == MIRType_Int32 && !range()->hasInt32UpperBound())
        range()->extendUInt32ToInt32Min();
}

void
MAbs::computeRange()
{
    if (specialization_ != MIRType_Int32 && specialization_ != MIRType_Double)
        return;

    Range other(getOperand(0));
    Range *next = Range::abs(&other);
    if (implicitTruncate_)
        next->wrapAroundToInt32();
    setRange(next);
}

void
MMinMax::computeRange()
{
    if (specialization_ != MIRType_Int32 && specialization_ != MIRType_Double)
        return;

    Range left(getOperand(0));
    Range right(getOperand(1));
    setRange(isMax() ? Range::max(&left, &right) : Range::min(&left, &right));
}

void
MAdd::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    Range *next = Range::add(&left, &right);
    if (isTruncated())
        next->wrapAroundToInt32();
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
    if (isTruncated())
        next->wrapAroundToInt32();
    setRange(next);
}

void
MMul::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    if (canBeNegativeZero())
        canBeNegativeZero_ = Range::negativeZeroMul(&left, &right);
    Range *next = Range::mul(&left, &right);
    
    if (isTruncated())
        next->wrapAroundToInt32();
    setRange(next);
}

void
MMod::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range lhs(getOperand(0));
    Range rhs(getOperand(1));

    
    
    if (!lhs.hasInt32Bounds() || !rhs.hasInt32Bounds())
        return;

    
    if (rhs.lower() <= 0 && rhs.upper() >= 0)
        return;

    
    
    
    int64_t a = Abs<int64_t>(rhs.lower());
    int64_t b = Abs<int64_t>(rhs.upper());
    if (a == 0 && b == 0)
        return;
    int64_t rhsAbsBound = Max(a, b);

    
    
    
    if (!lhs.canHaveFractionalPart() && !rhs.canHaveFractionalPart())
        --rhsAbsBound;

    
    
    int64_t lhsAbsBound = Max(Abs<int64_t>(lhs.lower()), Abs<int64_t>(lhs.upper()));

    
    int64_t absBound = Min(lhsAbsBound, rhsAbsBound);

    
    
    
    int64_t lower = lhs.lower() >= 0 ? 0 : -absBound;
    int64_t upper = lhs.upper() <= 0 ? 0 : absBound;

    setRange(new Range(lower, upper, lhs.canHaveFractionalPart() || rhs.canHaveFractionalPart(),
                       Min(lhs.exponent(), rhs.exponent())));
}

void
MDiv::computeRange()
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range lhs(getOperand(0));
    Range rhs(getOperand(1));

    
    
    if (!lhs.hasInt32Bounds() || !rhs.hasInt32Bounds())
        return;

    
    
    if (rhs.lower() > 0 && lhs.lower() >= 0)
        setRange(new Range(0, lhs.upper(), true, lhs.exponent()));
}

void
MSqrt::computeRange()
{
    Range input(getOperand(0));

    
    
    if (!input.hasInt32Bounds())
        return;

    
    if (input.lower() < 0)
        return;

    
    
    setRange(new Range(0, input.upper(), true, input.exponent()));
}

void
MToDouble::computeRange()
{
    setRange(new Range(getOperand(0)));
}

void
MToFloat32::computeRange()
{
    setRange(new Range(getOperand(0)));
}

void
MTruncateToInt32::computeRange()
{
    Range *output = new Range(getOperand(0));
    output->wrapAroundToInt32();
    setRange(output);
}

void
MToInt32::computeRange()
{
    Range *output = new Range(getOperand(0));
    output->clampToInt32();
    setRange(output);
}

static Range *GetTypedArrayRange(int type)
{
    switch (type) {
      case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
      case ScalarTypeRepresentation::TYPE_UINT8:
        return Range::NewUInt32Range(0, UINT8_MAX);
      case ScalarTypeRepresentation::TYPE_UINT16:
        return Range::NewUInt32Range(0, UINT16_MAX);
      case ScalarTypeRepresentation::TYPE_UINT32:
        return Range::NewUInt32Range(0, UINT32_MAX);

      case ScalarTypeRepresentation::TYPE_INT8:
        return Range::NewInt32Range(INT8_MIN, INT8_MAX);
      case ScalarTypeRepresentation::TYPE_INT16:
        return Range::NewInt32Range(INT16_MIN, INT16_MAX);
      case ScalarTypeRepresentation::TYPE_INT32:
        return Range::NewInt32Range(INT32_MIN, INT32_MAX);

      case ScalarTypeRepresentation::TYPE_FLOAT32:
      case ScalarTypeRepresentation::TYPE_FLOAT64:
        break;
    }

  return nullptr;
}

void
MLoadTypedArrayElement::computeRange()
{
    
    
    setRange(GetTypedArrayRange(arrayType()));
}

void
MLoadTypedArrayElementStatic::computeRange()
{
    
    
    JS_ASSERT(typedArray_->type() != ScalarTypeRepresentation::TYPE_UINT32);

    setRange(GetTypedArrayRange(typedArray_->type()));
}

void
MArrayLength::computeRange()
{
    
    
    
    setRange(Range::NewUInt32Range(0, INT32_MAX));
}

void
MInitializedLength::computeRange()
{
    setRange(Range::NewUInt32Range(0, JSObject::NELEMENTS_LIMIT));
}

void
MTypedArrayLength::computeRange()
{
    setRange(Range::NewUInt32Range(0, INT32_MAX));
}

void
MStringLength::computeRange()
{
    static_assert(JSString::MAX_LENGTH <= UINT32_MAX,
                  "NewUInt32Range requires a uint32 value");
    setRange(Range::NewUInt32Range(0, JSString::MAX_LENGTH));
}

void
MArgumentsLength::computeRange()
{
    
    
    static_assert(SNAPSHOT_MAX_NARGS <= UINT32_MAX,
                  "NewUInt32Range requires a uint32 value");
    setRange(Range::NewUInt32Range(0, SNAPSHOT_MAX_NARGS));
}





bool
RangeAnalysis::markBlocksInLoopBody(MBasicBlock *header, MBasicBlock *backedge)
{
    Vector<MBasicBlock *, 16, IonAllocPolicy> worklist;

    
    header->mark();

    backedge->mark();
    if (!worklist.append(backedge))
        return false;

    
    
    while (!worklist.empty()) {
        MBasicBlock *current = worklist.popCopy();
        for (size_t i = 0; i < current->numPredecessors(); i++) {
            MBasicBlock *pred = current->getPredecessor(i);

            if (pred->isMarked())
                continue;

            pred->mark();
            if (!worklist.append(pred))
                return false;
        }
    }

    return true;
}

bool
RangeAnalysis::analyzeLoop(MBasicBlock *header)
{
    JS_ASSERT(header->hasUniqueBackedge());

    
    
    
    MBasicBlock *backedge = header->backedge();

    
    if (backedge == header)
        return true;

    if (!markBlocksInLoopBody(header, backedge))
        return false;

    LoopIterationBound *iterationBound = nullptr;

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
        return true;
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

    
    

    for (MPhiIterator iter(header->phisBegin()); iter != header->phisEnd(); iter++)
        analyzeLoopPhi(header, iterationBound, *iter);

    if (!mir->compilingAsmJS()) {
        

        Vector<MBoundsCheck *, 0, IonAllocPolicy> hoistedChecks;

        for (ReversePostorderIterator iter(graph_.rpoBegin(header)); iter != graph_.rpoEnd(); iter++) {
            MBasicBlock *block = *iter;
            if (!block->isMarked())
                continue;

            for (MDefinitionIterator iter(block); iter; iter++) {
                MDefinition *def = *iter;
                if (def->isBoundsCheck() && def->isMovable()) {
                    if (tryHoistBoundsCheck(header, def->toBoundsCheck())) {
                        if (!hoistedChecks.append(def->toBoundsCheck()))
                            return false;
                    }
                }
            }
        }

        
        
        
        
        
        for (size_t i = 0; i < hoistedChecks.length(); i++) {
            MBoundsCheck *ins = hoistedChecks[i];
            ins->replaceAllUsesWith(ins->index());
            ins->block()->discard(ins);
        }
    }

    graph_.unmarkBlocks();
    return true;
}

LoopIterationBound *
RangeAnalysis::analyzeLoopIterationCount(MBasicBlock *header,
                                         MTest *test, BranchDirection direction)
{
    SimpleLinearSum lhs(nullptr, 0);
    MDefinition *rhs;
    bool lessEqual;
    if (!ExtractLinearInequality(test, direction, &lhs, &rhs, &lessEqual))
        return nullptr;

    
    if (rhs && rhs->block()->isMarked()) {
        if (lhs.term && lhs.term->block()->isMarked())
            return nullptr;
        MDefinition *temp = lhs.term;
        lhs.term = rhs;
        rhs = temp;
        if (!SafeSub(0, lhs.constant, &lhs.constant))
            return nullptr;
        lessEqual = !lessEqual;
    }

    JS_ASSERT_IF(rhs, !rhs->block()->isMarked());

    
    if (!lhs.term || !lhs.term->isPhi() || lhs.term->block() != header)
        return nullptr;

    
    
    
    

    if (lhs.term->toPhi()->numOperands() != 2)
        return nullptr;

    
    
    
    MDefinition *lhsInitial = lhs.term->toPhi()->getOperand(0);
    if (lhsInitial->block()->isMarked())
        return nullptr;

    
    
    MDefinition *lhsWrite = lhs.term->toPhi()->getOperand(1);
    if (lhsWrite->isBeta())
        lhsWrite = lhsWrite->getOperand(0);
    if (!lhsWrite->isAdd() && !lhsWrite->isSub())
        return nullptr;
    if (!lhsWrite->block()->isMarked())
        return nullptr;
    MBasicBlock *bb = header->backedge();
    for (; bb != lhsWrite->block() && bb != header; bb = bb->immediateDominator()) {}
    if (bb != lhsWrite->block())
        return nullptr;

    SimpleLinearSum lhsModified = ExtractLinearSum(lhsWrite);

    
    
    
    
    
    
    
    if (lhsModified.term != lhs.term)
        return nullptr;

    LinearSum bound;

    if (lhsModified.constant == 1 && !lessEqual) {
        
        
        
        
        
        

        if (rhs) {
            if (!bound.add(rhs, 1))
                return nullptr;
        }
        if (!bound.add(lhsInitial, -1))
            return nullptr;

        int32_t lhsConstant;
        if (!SafeSub(0, lhs.constant, &lhsConstant))
            return nullptr;
        if (!bound.add(lhsConstant))
            return nullptr;
    } else if (lhsModified.constant == -1 && lessEqual) {
        
        
        
        
        

        if (!bound.add(lhsInitial, 1))
            return nullptr;
        if (rhs) {
            if (!bound.add(rhs, -1))
                return nullptr;
        }
        if (!bound.add(lhs.constant))
            return nullptr;
    } else {
        return nullptr;
    }

    return new LoopIterationBound(header, test, bound);
}

void
RangeAnalysis::analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi)
{
    
    
    
    
    
    

    JS_ASSERT(phi->numOperands() == 2);

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

    Range *initRange = initial->range();
    if (modified.constant > 0) {
        if (initRange && initRange->hasInt32LowerBound())
            phi->range()->refineLower(initRange->lower());
        phi->range()->setSymbolicLower(new SymbolicBound(nullptr, initialSum));
        phi->range()->setSymbolicUpper(new SymbolicBound(loopBound, limitSum));
    } else {
        if (initRange && initRange->hasInt32UpperBound())
            phi->range()->refineUpper(initRange->upper());
        phi->range()->setSymbolicUpper(new SymbolicBound(nullptr, initialSum));
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
    MDefinition *def = nullptr;

    for (size_t i = 0; i < sum.numTerms(); i++) {
        LinearTerm term = sum.term(i);
        JS_ASSERT(!term.term->isConstant());
        if (term.scale == 1) {
            if (def) {
                def = MAdd::New(def, term.term);
                def->toAdd()->setInt32();
                block->insertBefore(block->lastIns(), def->toInstruction());
                def->computeRange();
            } else {
                def = term.term;
            }
        } else if (term.scale == -1) {
            if (!def) {
                def = MConstant::New(Int32Value(0));
                block->insertBefore(block->lastIns(), def->toInstruction());
                def->computeRange();
            }
            def = MSub::New(def, term.term);
            def->toSub()->setInt32();
            block->insertBefore(block->lastIns(), def->toInstruction());
            def->computeRange();
        } else {
            JS_ASSERT(term.scale != 0);
            MConstant *factor = MConstant::New(Int32Value(term.scale));
            block->insertBefore(block->lastIns(), factor);
            MMul *mul = MMul::New(term.term, factor);
            mul->setInt32();
            block->insertBefore(block->lastIns(), mul);
            mul->computeRange();
            if (def) {
                def = MAdd::New(def, mul);
                def->toAdd()->setInt32();
                block->insertBefore(block->lastIns(), def->toInstruction());
                def->computeRange();
            } else {
                def = mul;
            }
        }
    }

    if (!def) {
        def = MConstant::New(Int32Value(0));
        block->insertBefore(block->lastIns(), def->toInstruction());
        def->computeRange();
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

    
    
    
    

    int32_t upperConstant = index.constant;
    if (!SafeAdd(upper->sum.constant(), upperConstant, &upperConstant))
        return false;

    MBoundsCheckLower *lowerCheck = MBoundsCheckLower::New(lowerTerm);
    lowerCheck->setMinimum(lowerConstant);

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

        if (block->isLoopHeader()) {
            if (!analyzeLoop(block))
                return false;
        }

        if (mir->compilingAsmJS()) {
            for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
                if (i->isAsmJSLoadHeap()) {
                    MAsmJSLoadHeap *ins = i->toAsmJSLoadHeap();
                    Range *range = ins->ptr()->range();
                    if (range && range->hasInt32LowerBound() && range->lower() >= 0 &&
                        range->hasInt32UpperBound() &&
                        (uint32_t) range->upper() < mir->minAsmJSHeapLength())
                        ins->setSkipBoundsCheck(true);
                } else if (i->isAsmJSStoreHeap()) {
                    MAsmJSStoreHeap *ins = i->toAsmJSStoreHeap();
                    Range *range = ins->ptr()->range();
                    if (range && range->hasInt32LowerBound() && range->lower() >= 0 &&
                        range->hasInt32UpperBound() &&
                        (uint32_t) range->upper() < mir->minAsmJSHeapLength())
                        ins->setSkipBoundsCheck(true);
                }
            }
        }
    }

    return true;
}

bool
RangeAnalysis::addRangeAssertions()
{
    if (!js_IonOptions.checkRangeAnalysis)
        return true;

    
    
    
    
    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;

        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
            MInstruction *ins = *iter;

            if (ins->type() == MIRType_None)
                continue;

            Range *r = ins->range();
            if (!r)
                continue;

            MAssertRange *guard = MAssertRange::New(ins, new Range(*r));

            
            
            
            MInstructionIterator insertIter = iter;
            while (insertIter->isBeta())
                insertIter++;

            if (*insertIter == *iter)
                block->insertAfter(*insertIter,  guard);
            else
                block->insertBefore(*insertIter, guard);
        }
    }

    return true;
}





void
Range::clampToInt32()
{
    if (isInt32())
        return;
    int32_t l = hasInt32LowerBound() ? lower() : JSVAL_INT_MIN;
    int32_t h = hasInt32UpperBound() ? upper() : JSVAL_INT_MAX;
    setInt32(l, h);
}

void
Range::wrapAroundToInt32()
{
    if (!hasInt32Bounds())
        setInt32(JSVAL_INT_MIN, JSVAL_INT_MAX);
    else if (canHaveFractionalPart())
        canHaveFractionalPart_ = false;
}

void
Range::wrapAroundToShiftCount()
{
    wrapAroundToInt32();
    if (lower() < 0 || upper() >= 32)
        setInt32(0, 31);
}

void
Range::wrapAroundToBoolean()
{
    wrapAroundToInt32();
    if (!isBoolean())
        setInt32(0, 1);
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

    
    int32_t res = ToInt32(value_.toDouble());
    value_.setInt32(res);
    setResultType(MIRType_Int32);
    if (range())
        range()->setInt32(res, res);
    return true;
}

bool
MAdd::truncate()
{
    
    setTruncated(true);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MSub::truncate()
{
    
    setTruncated(true);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MMul::truncate()
{
    
    setTruncated(true);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        setCanBeNegativeZero(false);
        if (range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MDiv::truncate()
{
    
    setTruncated(true);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (range())
            range()->wrapAroundToInt32();

        
        
        if (tryUseUnsignedOperands())
            unsigned_ = true;

        return true;
    }

    JS_ASSERT(specialization() != MIRType_Int32); 

    
    return false;
}

bool
MMod::truncate()
{
    
    setTruncated(true);

    
    if (specialization() == MIRType_Int32 && tryUseUnsignedOperands()) {
        unsigned_ = true;
        return true;
    }

    
    return false;
}

bool
MToDouble::truncate()
{
    JS_ASSERT(type() == MIRType_Double);

    
    
    setResultType(MIRType_Int32);
    if (range())
        range()->wrapAroundToInt32();

    return true;
}

bool
MToFloat32::truncate()
{
    JS_ASSERT(type() == MIRType_Float32);

    
    
    setResultType(MIRType_Int32);
    if (range())
        range()->wrapAroundToInt32();

    return true;
}

bool
MLoadTypedArrayElementStatic::truncate()
{
    setInfallible();
    return false;
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

bool
MToFloat32::isOperandTruncated(size_t index) const
{
    
    
    return type() == MIRType_Int32;
}

bool
MStoreTypedArrayElement::isOperandTruncated(size_t index) const
{
    return index == 2 && !isFloatArray();
}

bool
MStoreTypedArrayElementHole::isOperandTruncated(size_t index) const
{
    return index == 3 && !isFloatArray();
}

bool
MStoreTypedArrayElementStatic::isOperandTruncated(size_t index) const
{
    return index == 1 && !isFloatArray();
}



static bool
AllUsesTruncate(MInstruction *candidate)
{
    for (MUseIterator use(candidate->usesBegin()); use != candidate->usesEnd(); use++) {
        if (!use->consumer()->isDefinition()) {
            
            
            if (candidate->isUseRemoved())
                return false;
            continue;
        }

        if (!use->consumer()->toDefinition()->isOperandTruncated(use->index()))
            return false;
    }

    return true;
}

static void
RemoveTruncatesOnOutput(MInstruction *truncated)
{
    JS_ASSERT(truncated->type() == MIRType_Int32);
    JS_ASSERT(Range(truncated).isInt32());

    for (MUseDefIterator use(truncated); use; use++) {
        MDefinition *def = use.def();
        if (!def->isTruncateToInt32() || !def->isToInt32())
            continue;

        def->replaceAllUsesWith(truncated);
    }
}

static void
AdjustTruncatedInputs(MInstruction *truncated)
{
    MBasicBlock *block = truncated->block();
    for (size_t i = 0, e = truncated->numOperands(); i < e; i++) {
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
    Vector<MBinaryBitwiseInstruction *, 16, SystemAllocPolicy> bitops;

    for (PostorderIterator block(graph_.poBegin()); block != graph_.poEnd(); block++) {
        for (MInstructionReverseIterator iter(block->rbegin()); iter != block->rend(); iter++) {
            if (iter->type() == MIRType_None)
                continue;

            
            switch (iter->op()) {
              case MDefinition::Op_BitAnd:
              case MDefinition::Op_BitOr:
              case MDefinition::Op_BitXor:
              case MDefinition::Op_Lsh:
              case MDefinition::Op_Rsh:
              case MDefinition::Op_Ursh:
                if (!bitops.append(static_cast<MBinaryBitwiseInstruction*>(*iter)))
                    return false;
              default:;
            }

            
            
            
            
            const Range *r = iter->range();
            bool canHaveRoundingErrors = !r || r->canHaveRoundingErrors();

            
            
            if (iter->isDiv() && iter->toDiv()->specialization() == MIRType_Int32)
                canHaveRoundingErrors = false;

            if (canHaveRoundingErrors)
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

    
    
    
    
    
    
    
    
    
    
    
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++)
            iter->collectRangeInfo();
    }

    
    
    
    for (size_t i = 0; i < bitops.length(); i++) {
        MBinaryBitwiseInstruction *ins = bitops[i];
        MDefinition *folded = ins->foldUnnecessaryBitop();
        if (folded != ins)
            ins->replaceAllUsesWith(folded);
    }

    return true;
}





void
MInArray::collectRangeInfo()
{
    Range indexRange(index());
    needsNegativeIntCheck_ = !indexRange.isFiniteNonNegative();
}

void
MLoadElementHole::collectRangeInfo()
{
    Range indexRange(index());
    needsNegativeIntCheck_ = !indexRange.isFiniteNonNegative();
}

void
MMod::collectRangeInfo()
{
    Range lhsRange(lhs());
    canBeNegativeDividend_ = !lhsRange.isFiniteNonNegative();
}

void
MBoundsCheckLower::collectRangeInfo()
{
    Range indexRange(index());
    fallible_ = !indexRange.hasInt32LowerBound() || indexRange.lower() < minimum_;
}
