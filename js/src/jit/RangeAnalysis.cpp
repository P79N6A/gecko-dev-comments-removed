





#include "jit/RangeAnalysis.h"

#include "mozilla/MathAlgorithms.h"

#include "jsanalyze.h"

#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "vm/NumericConversions.h"

using namespace js;
using namespace js::jit;

using mozilla::Abs;
using mozilla::CountLeadingZeroes32;
using mozilla::ExponentComponent;
using mozilla::IsInfinite;
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
        } else if (left->type() == MIRType_Int32 && right->type() == MIRType_Int32) {
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
    JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
    JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);

    
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

    int32_t newLower = Max(lhs->lower_, rhs->lower_);
    int32_t newUpper = Min(lhs->upper_, rhs->upper_);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (newUpper < newLower) {
        *emptyRange = true;
        return NULL;
    }

    Range *r = new Range(
        newLower, newUpper,
        lhs->canHaveFractionalPart_ && rhs->canHaveFractionalPart_,
        Min(lhs->max_exponent_, rhs->max_exponent_));

    r->hasInt32LowerBound_ = lhs->hasInt32LowerBound_ || rhs->hasInt32LowerBound_;
    r->hasInt32UpperBound_ = lhs->hasInt32UpperBound_ || rhs->hasInt32UpperBound_;

    return r;
}

void
Range::unionWith(const Range *other)
{
    bool canHaveFractionalPart = canHaveFractionalPart_ | other->canHaveFractionalPart_;
    uint16_t max_exponent = Max(max_exponent_, other->max_exponent_);

    if (!hasInt32LowerBound_ || !other->hasInt32LowerBound_)
        dropInt32LowerBound();
    else
        setLowerInit(Min(lower_, other->lower_));

    if (!hasInt32UpperBound_ || !other->hasInt32UpperBound_)
        dropInt32UpperBound();
    else
        setUpperInit(Max(upper_, other->upper_));

    canHaveFractionalPart_ = canHaveFractionalPart;
    max_exponent_ = max_exponent;
}

Range::Range(const MDefinition *def)
  : symbolicLower_(NULL),
    symbolicUpper_(NULL)
{
    const Range *other = def->range();
    if (!other) {
        if (def->type() == MIRType_Int32)
            set(JSVAL_INT_MIN, JSVAL_INT_MAX);
        else if (def->type() == MIRType_Boolean)
            set(0, 1);
        else
            set(NoInt32LowerBound, NoInt32UpperBound, true, MaxDoubleExponent);
        symbolicLower_ = symbolicUpper_ = NULL;
        return;
    }

    *this = *other;
    symbolicLower_ = symbolicUpper_ = NULL;

    if (def->type() == MIRType_Boolean)
        wrapAroundToBoolean();
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

    return new Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(),
                     Max(lhs->exponent(), rhs->exponent()) + 1);
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

    return new Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(),
                     Max(lhs->exponent(), rhs->exponent()) + 1);
}

Range *
Range::and_(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    int64_t lower;
    int64_t upper;

    
    if (lhs->lower_ < 0 && rhs->lower_ < 0) {
        lower = INT_MIN;
        upper = Max(lhs->upper_, rhs->upper_);
        return Range::NewInt32Range(lower, upper);
    }

    
    
    
    lower = 0;
    upper = Min(lhs->upper_, rhs->upper_);

    
    
    
    if (lhs->lower_ < 0)
       upper = rhs->upper_;
    if (rhs->lower_ < 0)
        upper = lhs->upper_;

    return Range::NewInt32Range(lower, upper);
}

Range *
Range::or_(const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    
    
    
    
    if (lhs->lower_ == lhs->upper_) {
        if (lhs->lower_ == 0)
            return new Range(*rhs);
        if (lhs->lower_ == -1)
            return new Range(*lhs);;
    }
    if (rhs->lower_ == rhs->upper_) {
        if (rhs->lower_ == 0)
            return new Range(*lhs);
        if (rhs->lower_ == -1)
            return new Range(*rhs);;
    }

    
    
    JS_ASSERT_IF(lhs->lower_ >= 0, lhs->upper_ != 0);
    JS_ASSERT_IF(rhs->lower_ >= 0, rhs->upper_ != 0);
    JS_ASSERT_IF(lhs->upper_ < 0, lhs->lower_ != -1);
    JS_ASSERT_IF(rhs->upper_ < 0, rhs->lower_ != -1);

    int64_t lower = INT32_MIN;
    int64_t upper = INT32_MAX;

    if (lhs->lower_ >= 0 && rhs->lower_ >= 0) {
        
        lower = Max(lhs->lower_, rhs->lower_);
        
        upper = UINT32_MAX >> Min(CountLeadingZeroes32(lhs->upper_),
                                  CountLeadingZeroes32(rhs->upper_));
    } else {
        
        if (lhs->upper_ < 0) {
            unsigned leadingOnes = CountLeadingZeroes32(~lhs->lower_);
            lower = Max(lower, int64_t(~int32_t(UINT32_MAX >> leadingOnes)));
            upper = -1;
        }
        if (rhs->upper_ < 0) {
            unsigned leadingOnes = CountLeadingZeroes32(~rhs->lower_);
            lower = Max(lower, int64_t(~int32_t(UINT32_MAX >> leadingOnes)));
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
    int32_t lhsLower = lhs->lower_;
    int32_t lhsUpper = lhs->upper_;
    int32_t rhsLower = rhs->lower_;
    int32_t rhsUpper = rhs->upper_;
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
    return Range::NewInt32Range(~op->upper_, ~op->lower_);
}

Range *
Range::mul(const Range *lhs, const Range *rhs)
{
    bool fractional = lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart();
    uint16_t exponent = lhs->numBits() + rhs->numBits() - 1;
    if (MissingAnyInt32Bounds(lhs, rhs))
        return new Range(NoInt32LowerBound, NoInt32UpperBound, fractional, exponent);
    int64_t a = (int64_t)lhs->lower_ * (int64_t)rhs->lower_;
    int64_t b = (int64_t)lhs->lower_ * (int64_t)rhs->upper_;
    int64_t c = (int64_t)lhs->upper_ * (int64_t)rhs->lower_;
    int64_t d = (int64_t)lhs->upper_ * (int64_t)rhs->upper_;
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

    
    
    if ((int32_t)((uint32_t)lhs->lower_ << shift << 1 >> shift >> 1) == lhs->lower_ &&
        (int32_t)((uint32_t)lhs->upper_ << shift << 1 >> shift >> 1) == lhs->upper_)
    {
        return Range::NewInt32Range(
            (uint32_t)lhs->lower_ << shift,
            (uint32_t)lhs->upper_ << shift);
    }

    return Range::NewInt32Range(INT32_MIN, INT32_MAX);
}

Range *
Range::rsh(const Range *lhs, int32_t c)
{
    JS_ASSERT(lhs->isInt32());
    int32_t shift = c & 0x1f;
    return Range::NewInt32Range(
        lhs->lower_ >> shift,
        lhs->upper_ >> shift);
}

Range *
Range::ursh(const Range *lhs, int32_t c)
{
    int32_t shift = c & 0x1f;

    
    
    if ((lhs->lower_ >= 0 && lhs->hasInt32UpperBound()) ||
        (lhs->upper_ < 0 && lhs->hasInt32LowerBound()))
    {
        return Range::NewUInt32Range(
            uint32_t(lhs->lower_) >> shift,
            uint32_t(lhs->upper_) >> shift);
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
    return Range::NewUInt32Range(0, (lhs->lower() >= 0 && lhs->hasInt32UpperBound()) ? lhs->upper() : UINT32_MAX);
}

Range *
Range::abs(const Range *op)
{
    
    
    
    int64_t l = (int64_t)op->lower() - !op->hasInt32LowerBound();
    int64_t u = (int64_t)op->upper() + !op->hasInt32UpperBound();

    return new Range(Max(Max(int64_t(0), l), -u),
                     Max(Abs(l), Abs(u)),
                     op->canHaveFractionalPart(),
                     op->exponent());
}

Range *
Range::min(const Range *lhs, const Range *rhs)
{
    
    if (!lhs->hasInt32Bounds() || !rhs->hasInt32Bounds())
        return new Range();

    return new Range(Min(lhs->lower(), rhs->lower()),
                     Min(lhs->upper(), rhs->upper()),
                     lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(),
                     Max(lhs->exponent(), rhs->exponent()));
}

Range *
Range::max(const Range *lhs, const Range *rhs)
{
    
    if (!lhs->hasInt32Bounds() || !rhs->hasInt32Bounds())
        return new Range();

    return new Range(Max(lhs->lower(), rhs->lower()),
                     Max(lhs->upper(), rhs->upper()),
                     lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(),
                     Max(lhs->exponent(), rhs->exponent()));
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
    for (size_t i = 0, e = numOperands(); i < e; i++) {
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
}

void
MBeta::computeRange()
{
    bool emptyRange = false;

    Range *range = Range::intersect(getOperand(0)->range(), comparison_, &emptyRange);
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
    int exp = Range::MaxDoubleExponent;

    
    if (IsNaN(d))
        return;

    
    if (IsInfinite(d)) {
        if (IsNegative(d))
            setRange(new Range(Range::NoInt32LowerBound, Range::NoInt32LowerBound, false, exp));
        else
            setRange(new Range(Range::NoInt32UpperBound, Range::NoInt32UpperBound, false, exp));
        return;
    }

    
    exp = ExponentComponent(d);
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
            setRange(new Range(Range::NoInt32LowerBound, Range::NoInt32LowerBound, false, exp));
        else
            setRange(new Range(Range::NoInt32UpperBound, Range::NoInt32UpperBound, false, exp));
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
    setRange(Range::abs(&other));

    if (implicitTruncate_)
        range()->wrapAroundToInt32();
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
    setRange(next);

    if (isTruncated())
        range()->wrapAroundToInt32();
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

    if (isTruncated())
        range()->wrapAroundToInt32();
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
    setRange(Range::mul(&left, &right));

    
    if (isTruncated())
        range()->wrapAroundToInt32();
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

    setRange(new Range(lower, upper, lhs.canHaveFractionalPart() || rhs.canHaveFractionalPart()));
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

  return NULL;
}

void
MLoadTypedArrayElement::computeRange()
{
    if (Range *range = GetTypedArrayRange(arrayType())) {
        if (type() == MIRType_Int32 && !range->hasInt32UpperBound())
            range->extendUInt32ToInt32Min();
        setRange(range);
    }
}

void
MLoadTypedArrayElementStatic::computeRange()
{
    if (Range *range = GetTypedArrayRange(typedArray_->type()))
        setRange(range);
}

void
MArrayLength::computeRange()
{
    Range *r = Range::NewUInt32Range(0, UINT32_MAX);
    r->extendUInt32ToInt32Min();
    setRange(r);
}

void
MInitializedLength::computeRange()
{
    Range *r = Range::NewUInt32Range(0, UINT32_MAX);
    r->extendUInt32ToInt32Min();
    setRange(r);
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
    JS_ASSERT(header->hasUniqueBackedge());

    
    
    
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
            phi->range()->setLower(initRange->lower());
        phi->range()->setSymbolicLower(new SymbolicBound(NULL, initialSum));
        phi->range()->setSymbolicUpper(new SymbolicBound(loopBound, limitSum));
    } else {
        if (initRange && initRange->hasInt32UpperBound())
            phi->range()->setUpper(initRange->upper());
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

        if (block->isLoopHeader())
            analyzeLoop(block);

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

            Range *r = ins->range();
            if (!r || ins->isAssertRange() || ins->isBeta())
                continue;

            MAssertRange *guard = MAssertRange::New(ins);
            guard->setRange(new Range(*r));
            block->insertAfter(ins, guard);
        }
    }

    return true;
}





void
Range::clampToInt32()
{
    if (isInt32())
        return;
    int64_t l = hasInt32LowerBound() ? lower() : JSVAL_INT_MIN;
    int64_t h = hasInt32UpperBound() ? upper() : JSVAL_INT_MAX;
    set(l, h);
}

void
Range::wrapAroundToInt32()
{
    if (!hasInt32Bounds())
        set(JSVAL_INT_MIN, JSVAL_INT_MAX);
    else if (canHaveFractionalPart())
        set(lower(), upper(), false, exponent());
}

void
Range::wrapAroundToShiftCount()
{
    if (lower() < 0 || upper() >= 32)
        set(0, 31);
}

void
Range::wrapAroundToBoolean()
{
    if (!isBoolean())
        set(0, 1);
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
        range()->set(res, res);
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
    needsNegativeIntCheck_ = !index()->range() || index()->range()->lower() < 0;
}

void
MLoadElementHole::collectRangeInfo()
{
    needsNegativeIntCheck_ = !index()->range() || index()->range()->lower() < 0;
}

void
MMod::collectRangeInfo()
{
    canBeNegativeDividend_ = !lhs()->range() || lhs()->range()->lower() < 0;
}

void
MBoundsCheckLower::collectRangeInfo()
{
    fallible_ = !index()->range() || index()->range()->lower() < minimum_;
}
