





#include "jit/RangeAnalysis.h"

#include "mozilla/MathAlgorithms.h"

#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/JitSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "vm/NumericConversions.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::Abs;
using mozilla::CountLeadingZeroes32;
using mozilla::NumberEqualsInt32;
using mozilla::ExponentComponent;
using mozilla::FloorLog2;
using mozilla::IsInfinite;
using mozilla::IsNaN;
using mozilla::IsNegative;
using mozilla::NegativeInfinity;
using mozilla::PositiveInfinity;
using mozilla::Swap;
using JS::GenericNaN;



























































static bool
IsDominatedUse(MBasicBlock *block, MUse *use)
{
    MNode *n = use->consumer();
    bool isPhi = n->isDefinition() && n->toDefinition()->isPhi();

    if (isPhi) {
        MPhi *phi = n->toDefinition()->toPhi();
        return block->dominates(phi->block()->getPredecessor(phi->indexOf(use)));
    }

    return block->dominates(n->block());
}

static inline void
SpewRange(MDefinition *def)
{
#ifdef DEBUG
    if (JitSpewEnabled(JitSpew_Range) && def->type() != MIRType_None && def->range()) {
        JitSpewHeader(JitSpew_Range);
        def->printName(JitSpewFile);
        fprintf(JitSpewFile, " has range ");
        def->range()->dump(JitSpewFile);
    }
#endif
}

TempAllocator &
RangeAnalysis::alloc() const
{
    return graph_.alloc();
}

void
RangeAnalysis::replaceDominatedUsesWith(MDefinition *orig, MDefinition *dom,
                                            MBasicBlock *block)
{
    for (MUseIterator i(orig->usesBegin()); i != orig->usesEnd(); ) {
        MUse *use = *i++;
        if (use->consumer() != dom && IsDominatedUse(block, use))
            use->replaceProducer(dom);
    }
}

bool
RangeAnalysis::addBetaNodes()
{
    JitSpew(JitSpew_Range, "Adding beta nodes");

    for (PostorderIterator i(graph_.poBegin()); i != graph_.poEnd(); i++) {
        MBasicBlock *block = *i;
        JitSpew(JitSpew_Range, "Looking at block %d", block->id());

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
        double conservativeLower = NegativeInfinity<double>();
        double conservativeUpper = PositiveInfinity<double>();
        MDefinition *val = nullptr;

        JSOp jsop = compare->jsop();

        if (branch_dir == FALSE_BRANCH) {
            jsop = NegateCompareOp(jsop);
            conservativeLower = GenericNaN();
            conservativeUpper = GenericNaN();
        }

        if (left->isConstant() && left->toConstant()->value().isNumber()) {
            bound = left->toConstant()->value().toNumber();
            val = right;
            jsop = ReverseCompareOp(jsop);
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
                beta = MBeta::New(alloc(), smaller,
                                  Range::NewInt32Range(alloc(), JSVAL_INT_MIN, JSVAL_INT_MAX-1));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(smaller, beta, block);
                JitSpew(JitSpew_Range, "Adding beta node for smaller %d", smaller->id());
                beta = MBeta::New(alloc(), greater,
                                  Range::NewInt32Range(alloc(), JSVAL_INT_MIN+1, JSVAL_INT_MAX));
                block->insertBefore(*block->begin(), beta);
                replaceDominatedUsesWith(greater, beta, block);
                JitSpew(JitSpew_Range, "Adding beta node for greater %d", greater->id());
            }
            continue;
        } else {
            continue;
        }

        
        
        JS_ASSERT(val);

        Range comp;
        switch (jsop) {
          case JSOP_LE:
            comp.setDouble(conservativeLower, bound);
            break;
          case JSOP_LT:
            
            if (val->type() == MIRType_Int32) {
                int32_t intbound;
                if (NumberEqualsInt32(bound, &intbound) && SafeSub(intbound, 1, &intbound))
                    bound = intbound;
            }
            comp.setDouble(conservativeLower, bound);
            break;
          case JSOP_GE:
            comp.setDouble(bound, conservativeUpper);
            break;
          case JSOP_GT:
            
            if (val->type() == MIRType_Int32) {
                int32_t intbound;
                if (NumberEqualsInt32(bound, &intbound) && SafeAdd(intbound, 1, &intbound))
                    bound = intbound;
            }
            comp.setDouble(bound, conservativeUpper);
            break;
          case JSOP_EQ:
            comp.setDouble(bound, bound);
            break;
          default:
            continue; 
                      
        }

        if (JitSpewEnabled(JitSpew_Range)) {
            JitSpewHeader(JitSpew_Range);
            fprintf(JitSpewFile, "Adding beta node for %d with range ", val->id());
            comp.dump(JitSpewFile);
        }

        MBeta *beta = MBeta::New(alloc(), val, new(alloc()) Range(comp));
        block->insertBefore(*block->begin(), beta);
        replaceDominatedUsesWith(val, beta, block);
    }

    return true;
}

bool
RangeAnalysis::removeBetaNodes()
{
    JitSpew(JitSpew_Range, "Removing beta nodes");

    for (PostorderIterator i(graph_.poBegin()); i != graph_.poEnd(); i++) {
        MBasicBlock *block = *i;
        for (MDefinitionIterator iter(*i); iter; ) {
            MDefinition *def = *iter;
            if (def->isBeta()) {
                MDefinition *op = def->getOperand(0);
                JitSpew(JitSpew_Range, "Removing beta node %d for %d",
                        def->id(), op->id());
                def->justReplaceAllUsesWith(op);
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
SymbolicBound::dump() const
{
    Sprinter sp(GetIonContext()->cx);
    sp.init();
    print(sp);
    fprintf(stderr, "%s\n", sp.string());
}



static bool
IsExponentInteresting(const Range *r)
{
   
   if (!r->hasInt32Bounds())
       return true;

   
   
   if (!r->canHaveFractionalPart())
       return false;

   
   
   return FloorLog2(Max(Abs(r->lower()), Abs(r->upper()))) > r->exponent();
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
    if (IsExponentInteresting(this)) {
        if (max_exponent_ == IncludesInfinityAndNaN)
            sp.printf(" (U inf U NaN)", max_exponent_);
        else if (max_exponent_ == IncludesInfinity)
            sp.printf(" (U inf)");
        else
            sp.printf(" (< pow(2, %d+1))", max_exponent_);
    }
}

void
Range::dump(FILE *fp) const
{
    Sprinter sp(GetIonContext()->cx);
    sp.init();
    print(sp);
    fprintf(fp, "%s\n", sp.string());
}

void
Range::dump() const
{
    dump(stderr);
}

Range *
Range::intersect(TempAllocator &alloc, const Range *lhs, const Range *rhs, bool *emptyRange)
{
    *emptyRange = false;

    if (!lhs && !rhs)
        return nullptr;

    if (!lhs)
        return new(alloc) Range(*rhs);
    if (!rhs)
        return new(alloc) Range(*lhs);

    int32_t newLower = Max(lhs->lower_, rhs->lower_);
    int32_t newUpper = Min(lhs->upper_, rhs->upper_);

    
    
    
    
    
    
    
    
    
    if (newUpper < newLower) {
        
        if (!lhs->canBeNaN() || !rhs->canBeNaN())
            *emptyRange = true;
        return nullptr;
    }

    bool newHasInt32LowerBound = lhs->hasInt32LowerBound_ || rhs->hasInt32LowerBound_;
    bool newHasInt32UpperBound = lhs->hasInt32UpperBound_ || rhs->hasInt32UpperBound_;
    bool newFractional = lhs->canHaveFractionalPart_ && rhs->canHaveFractionalPart_;
    uint16_t newExponent = Min(lhs->max_exponent_, rhs->max_exponent_);

    
    
    
    
    
    if (newHasInt32LowerBound && newHasInt32UpperBound && newExponent == IncludesInfinityAndNaN)
        return nullptr;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (lhs->canHaveFractionalPart_ != rhs->canHaveFractionalPart_ ||
        (lhs->canHaveFractionalPart_ &&
         newHasInt32LowerBound && newHasInt32UpperBound &&
         newLower == newUpper))
    {
        refineInt32BoundsByExponent(newExponent,
                                    &newLower, &newHasInt32LowerBound,
                                    &newUpper, &newHasInt32UpperBound);

        
        
        
        if (newLower > newUpper) {
            *emptyRange = true;
            return nullptr;
        }
    }

    return new(alloc) Range(newLower, newHasInt32LowerBound, newUpper, newHasInt32UpperBound,
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
    if (const Range *other = def->range()) {
        
        *this = *other;

        
        switch (def->type()) {
          case MIRType_Int32:
            wrapAroundToInt32();
            break;
          case MIRType_Boolean:
            wrapAroundToBoolean();
            break;
          case MIRType_None:
            MOZ_CRASH("Asking for the range of an instruction with no value");
          default:
            break;
        }
    } else {
        
        
        
        switch (def->type()) {
          case MIRType_Int32:
            setInt32(JSVAL_INT_MIN, JSVAL_INT_MAX);
            break;
          case MIRType_Boolean:
            setInt32(0, 1);
            break;
          case MIRType_None:
            MOZ_CRASH("Asking for the range of an instruction with no value");
          default:
            setUnknown();
            break;
        }
    }

    
    
    
    
    
    if (!hasInt32UpperBound() && def->isUrsh() && def->toUrsh()->bailoutsDisabled())
        lower_ = INT32_MIN;

    assertInvariants();
}

static uint16_t
ExponentImpliedByDouble(double d)
{
    
    if (IsNaN(d))
        return Range::IncludesInfinityAndNaN;
    if (IsInfinite(d))
        return Range::IncludesInfinity;

    
    
    return uint16_t(Max(int_fast16_t(0), ExponentComponent(d)));
}

void
Range::setDouble(double l, double h)
{
    
    if (l >= INT32_MIN && l <= INT32_MAX) {
        lower_ = int32_t(::floor(l));
        hasInt32LowerBound_ = true;
    } else {
        lower_ = INT32_MIN;
        hasInt32LowerBound_ = false;
    }
    if (h >= INT32_MIN && h <= INT32_MAX) {
        upper_ = int32_t(::ceil(h));
        hasInt32UpperBound_ = true;
    } else {
        upper_ = INT32_MAX;
        hasInt32UpperBound_ = false;
    }

    
    uint16_t lExp = ExponentImpliedByDouble(l);
    uint16_t hExp = ExponentImpliedByDouble(h);
    max_exponent_ = Max(lExp, hExp);

    
    
    
    
    uint16_t minExp = Min(lExp, hExp);
    bool includesNegative = IsNaN(l) || l < 0;
    bool includesPositive = IsNaN(h) || h > 0;
    bool crossesZero = includesNegative && includesPositive;
    canHaveFractionalPart_ = crossesZero || minExp < MaxTruncatableExponent;

    optimize();
}

static inline bool
MissingAnyInt32Bounds(const Range *lhs, const Range *rhs)
{
    return !lhs->hasInt32Bounds() || !rhs->hasInt32Bounds();
}

Range *
Range::add(TempAllocator &alloc, const Range *lhs, const Range *rhs)
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

    return new(alloc) Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(), e);
}

Range *
Range::sub(TempAllocator &alloc, const Range *lhs, const Range *rhs)
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

    return new(alloc) Range(l, h, lhs->canHaveFractionalPart() || rhs->canHaveFractionalPart(), e);
}

Range *
Range::and_(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());

    
    if (lhs->lower() < 0 && rhs->lower() < 0)
        return Range::NewInt32Range(alloc, INT32_MIN, Max(lhs->upper(), rhs->upper()));

    
    
    
    int32_t lower = 0;
    int32_t upper = Min(lhs->upper(), rhs->upper());

    
    
    
    if (lhs->lower() < 0)
       upper = rhs->upper();
    if (rhs->lower() < 0)
        upper = lhs->upper();

    return Range::NewInt32Range(alloc, lower, upper);
}

Range *
Range::or_(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    
    
    
    
    if (lhs->lower() == lhs->upper()) {
        if (lhs->lower() == 0)
            return new(alloc) Range(*rhs);
        if (lhs->lower() == -1)
            return new(alloc) Range(*lhs);
    }
    if (rhs->lower() == rhs->upper()) {
        if (rhs->lower() == 0)
            return new(alloc) Range(*lhs);
        if (rhs->lower() == -1)
            return new(alloc) Range(*rhs);
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

    return Range::NewInt32Range(alloc, lower, upper);
}

Range *
Range::xor_(TempAllocator &alloc, const Range *lhs, const Range *rhs)
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

    return Range::NewInt32Range(alloc, lower, upper);
}

Range *
Range::not_(TempAllocator &alloc, const Range *op)
{
    JS_ASSERT(op->isInt32());
    return Range::NewInt32Range(alloc, ~op->upper(), ~op->lower());
}

Range *
Range::mul(TempAllocator &alloc, const Range *lhs, const Range *rhs)
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
        return new(alloc) Range(NoInt32LowerBound, NoInt32UpperBound, fractional, exponent);
    int64_t a = (int64_t)lhs->lower() * (int64_t)rhs->lower();
    int64_t b = (int64_t)lhs->lower() * (int64_t)rhs->upper();
    int64_t c = (int64_t)lhs->upper() * (int64_t)rhs->lower();
    int64_t d = (int64_t)lhs->upper() * (int64_t)rhs->upper();
    return new(alloc) Range(
        Min( Min(a, b), Min(c, d) ),
        Max( Max(a, b), Max(c, d) ),
        fractional, exponent);
}

Range *
Range::lsh(TempAllocator &alloc, const Range *lhs, int32_t c)
{
    JS_ASSERT(lhs->isInt32());
    int32_t shift = c & 0x1f;

    
    
    if ((int32_t)((uint32_t)lhs->lower() << shift << 1 >> shift >> 1) == lhs->lower() &&
        (int32_t)((uint32_t)lhs->upper() << shift << 1 >> shift >> 1) == lhs->upper())
    {
        return Range::NewInt32Range(alloc,
            uint32_t(lhs->lower()) << shift,
            uint32_t(lhs->upper()) << shift);
    }

    return Range::NewInt32Range(alloc, INT32_MIN, INT32_MAX);
}

Range *
Range::rsh(TempAllocator &alloc, const Range *lhs, int32_t c)
{
    JS_ASSERT(lhs->isInt32());
    int32_t shift = c & 0x1f;
    return Range::NewInt32Range(alloc,
        lhs->lower() >> shift,
        lhs->upper() >> shift);
}

Range *
Range::ursh(TempAllocator &alloc, const Range *lhs, int32_t c)
{
    
    
    
    JS_ASSERT(lhs->isInt32());

    int32_t shift = c & 0x1f;

    
    
    if (lhs->isFiniteNonNegative() || lhs->isFiniteNegative()) {
        return Range::NewUInt32Range(alloc,
            uint32_t(lhs->lower()) >> shift,
            uint32_t(lhs->upper()) >> shift);
    }

    
    return Range::NewUInt32Range(alloc, 0, UINT32_MAX >> shift);
}

Range *
Range::lsh(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewInt32Range(alloc, INT32_MIN, INT32_MAX);
}

Range *
Range::rsh(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewInt32Range(alloc, Min(lhs->lower(), 0), Max(lhs->upper(), 0));
}

Range *
Range::ursh(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    
    
    
    JS_ASSERT(lhs->isInt32());
    JS_ASSERT(rhs->isInt32());
    return Range::NewUInt32Range(alloc, 0, lhs->isFiniteNonNegative() ? lhs->upper() : UINT32_MAX);
}

Range *
Range::abs(TempAllocator &alloc, const Range *op)
{
    int32_t l = op->lower_;
    int32_t u = op->upper_;

    return new(alloc) Range(Max(Max(int32_t(0), l), u == INT32_MIN ? INT32_MAX : -u),
                            true,
                            Max(Max(int32_t(0), u), l == INT32_MIN ? INT32_MAX : -l),
                            op->hasInt32Bounds() && l != INT32_MIN,
                            op->canHaveFractionalPart_,
                            op->max_exponent_);
}

Range *
Range::min(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    
    if (lhs->canBeNaN() || rhs->canBeNaN())
        return nullptr;

    return new(alloc) Range(Min(lhs->lower_, rhs->lower_),
                            lhs->hasInt32LowerBound_ && rhs->hasInt32LowerBound_,
                            Min(lhs->upper_, rhs->upper_),
                            lhs->hasInt32UpperBound_ || rhs->hasInt32UpperBound_,
                            lhs->canHaveFractionalPart_ || rhs->canHaveFractionalPart_,
                            Max(lhs->max_exponent_, rhs->max_exponent_));
}

Range *
Range::max(TempAllocator &alloc, const Range *lhs, const Range *rhs)
{
    
    if (lhs->canBeNaN() || rhs->canBeNaN())
        return nullptr;

    return new(alloc) Range(Max(lhs->lower_, rhs->lower_),
                            lhs->hasInt32LowerBound_ || rhs->hasInt32LowerBound_,
                            Max(lhs->upper_, rhs->upper_),
                            lhs->hasInt32UpperBound_ && rhs->hasInt32UpperBound_,
                            lhs->canHaveFractionalPart_ || rhs->canHaveFractionalPart_,
                            Max(lhs->max_exponent_, rhs->max_exponent_));
}

Range *
Range::floor(TempAllocator &alloc, const Range *op)
{
    Range *copy = new(alloc) Range(*op);
    
    
    
    if (op->canHaveFractionalPart() && op->hasInt32LowerBound())
        copy->setLowerInit(int64_t(copy->lower_) - 1);

    
    
    
    
    
    if(copy->hasInt32Bounds())
        copy->max_exponent_ = copy->exponentImpliedByInt32Bounds();
    else if(copy->max_exponent_ < MaxFiniteExponent)
        copy->max_exponent_++;

    copy->canHaveFractionalPart_ = false;
    copy->assertInvariants();
    return copy;
}

Range *
Range::ceil(TempAllocator &alloc, const Range *op)
{
    Range *copy = new(alloc) Range(*op);

    
    
    
    
    if (copy->hasInt32Bounds())
        copy->max_exponent_ = copy->exponentImpliedByInt32Bounds();
    else if (copy->max_exponent_ < MaxFiniteExponent)
        copy->max_exponent_++;

    copy->canHaveFractionalPart_ = false;
    copy->assertInvariants();
    return copy;
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
MPhi::computeRange(TempAllocator &alloc)
{
    if (type() != MIRType_Int32 && type() != MIRType_Double)
        return;

    Range *range = nullptr;
    for (size_t i = 0, e = numOperands(); i < e; i++) {
        if (getOperand(i)->block()->unreachable()) {
            JitSpew(JitSpew_Range, "Ignoring unreachable input %d", getOperand(i)->id());
            continue;
        }

        
        
        if (!getOperand(i)->range())
            return;

        Range input(getOperand(i));

        if (range)
            range->unionWith(&input);
        else
            range = new(alloc) Range(input);
    }

    setRange(range);
}

void
MBeta::computeRange(TempAllocator &alloc)
{
    bool emptyRange = false;

    Range opRange(getOperand(0));
    Range *range = Range::intersect(alloc, &opRange, comparison_, &emptyRange);
    if (emptyRange) {
        JitSpew(JitSpew_Range, "Marking block for inst %d unreachable", id());
        block()->setUnreachableUnchecked();
    } else {
        setRange(range);
    }
}

void
MConstant::computeRange(TempAllocator &alloc)
{
    if (value().isNumber()) {
        double d = value().toNumber();
        setRange(Range::NewDoubleRange(alloc, d, d));
    } else if (value().isBoolean()) {
        bool b = value().toBoolean();
        setRange(Range::NewInt32Range(alloc, b, b));
    }
}

void
MCharCodeAt::computeRange(TempAllocator &alloc)
{
    
    setRange(Range::NewInt32Range(alloc, 0, 65535));
}

void
MClampToUint8::computeRange(TempAllocator &alloc)
{
    setRange(Range::NewUInt32Range(alloc, 0, 255));
}

void
MBitAnd::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::and_(alloc, &left, &right));
}

void
MBitOr::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::or_(alloc, &left, &right));
}

void
MBitXor::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();
    right.wrapAroundToInt32();

    setRange(Range::xor_(alloc, &left, &right));
}

void
MBitNot::computeRange(TempAllocator &alloc)
{
    Range op(getOperand(0));
    op.wrapAroundToInt32();

    setRange(Range::not_(alloc, &op));
}

void
MLsh::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        right.wrapAroundToShiftCount();
        setRange(Range::lsh(alloc, &left, &right));
        return;
    }

    int32_t c = rhs->toConstant()->value().toInt32();
    setRange(Range::lsh(alloc, &left, c));
}

void
MRsh::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));
    left.wrapAroundToInt32();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        right.wrapAroundToShiftCount();
        setRange(Range::rsh(alloc, &left, &right));
        return;
    }

    int32_t c = rhs->toConstant()->value().toInt32();
    setRange(Range::rsh(alloc, &left, c));
}

void
MUrsh::computeRange(TempAllocator &alloc)
{
    Range left(getOperand(0));
    Range right(getOperand(1));

    
    
    
    
    
    left.wrapAroundToInt32();
    right.wrapAroundToShiftCount();

    MDefinition *rhs = getOperand(1);
    if (!rhs->isConstant()) {
        setRange(Range::ursh(alloc, &left, &right));
    } else {
        int32_t c = rhs->toConstant()->value().toInt32();
        setRange(Range::ursh(alloc, &left, c));
    }

    JS_ASSERT(range()->lower() >= 0);
}

void
MAbs::computeRange(TempAllocator &alloc)
{
    if (specialization_ != MIRType_Int32 && specialization_ != MIRType_Double)
        return;

    Range other(getOperand(0));
    Range *next = Range::abs(alloc, &other);
    if (implicitTruncate_)
        next->wrapAroundToInt32();
    setRange(next);
}

void
MFloor::computeRange(TempAllocator &alloc)
{
    Range other(getOperand(0));
    setRange(Range::floor(alloc, &other));
}

void
MCeil::computeRange(TempAllocator &alloc)
{
    Range other(getOperand(0));
    setRange(Range::ceil(alloc, &other));
}

void
MClz::computeRange(TempAllocator &alloc)
{
    setRange(Range::NewUInt32Range(alloc, 0, 32));
}

void
MMinMax::computeRange(TempAllocator &alloc)
{
    if (specialization_ != MIRType_Int32 && specialization_ != MIRType_Double)
        return;

    Range left(getOperand(0));
    Range right(getOperand(1));
    setRange(isMax() ? Range::max(alloc, &left, &right) : Range::min(alloc, &left, &right));
}

void
MAdd::computeRange(TempAllocator &alloc)
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    Range *next = Range::add(alloc, &left, &right);
    if (isTruncated())
        next->wrapAroundToInt32();
    setRange(next);
}

void
MSub::computeRange(TempAllocator &alloc)
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    Range *next = Range::sub(alloc, &left, &right);
    if (isTruncated())
        next->wrapAroundToInt32();
    setRange(next);
}

void
MMul::computeRange(TempAllocator &alloc)
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range left(getOperand(0));
    Range right(getOperand(1));
    if (canBeNegativeZero())
        canBeNegativeZero_ = Range::negativeZeroMul(&left, &right);
    Range *next = Range::mul(alloc, &left, &right);
    
    if (isTruncated())
        next->wrapAroundToInt32();
    setRange(next);
}

void
MMod::computeRange(TempAllocator &alloc)
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range lhs(getOperand(0));
    Range rhs(getOperand(1));

    
    
    if (!lhs.hasInt32Bounds() || !rhs.hasInt32Bounds())
        return;

    
    if (rhs.lower() <= 0 && rhs.upper() >= 0)
        return;

    
    
    if (specialization() == MIRType_Int32 && lhs.lower() >= 0 && rhs.lower() > 0 &&
        !lhs.canHaveFractionalPart() && !rhs.canHaveFractionalPart())
    {
        unsigned_ = true;
    }

    
    
    if (unsigned_) {
        
        
        uint32_t lhsBound = Max<uint32_t>(lhs.lower(), lhs.upper());
        uint32_t rhsBound = Max<uint32_t>(rhs.lower(), rhs.upper());

        
        
        
        
        if (lhs.lower() <= -1 && lhs.upper() >= -1)
            lhsBound = UINT32_MAX;
        if (rhs.lower() <= -1 && rhs.upper() >= -1)
            rhsBound = UINT32_MAX;

        
        
        JS_ASSERT(!lhs.canHaveFractionalPart() && !rhs.canHaveFractionalPart());
        --rhsBound;

        
        setRange(Range::NewUInt32Range(alloc, 0, Min(lhsBound, rhsBound)));
        return;
    }

    
    
    
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

    setRange(new(alloc) Range(lower, upper, lhs.canHaveFractionalPart() || rhs.canHaveFractionalPart(),
                              Min(lhs.exponent(), rhs.exponent())));
}

void
MDiv::computeRange(TempAllocator &alloc)
{
    if (specialization() != MIRType_Int32 && specialization() != MIRType_Double)
        return;
    Range lhs(getOperand(0));
    Range rhs(getOperand(1));

    
    
    if (!lhs.hasInt32Bounds() || !rhs.hasInt32Bounds())
        return;

    
    
    if (lhs.lower() >= 0 && rhs.lower() >= 1) {
        setRange(new(alloc) Range(0, lhs.upper(), true, lhs.exponent()));
    } else if (unsigned_ && rhs.lower() >= 1) {
        
        
        JS_ASSERT(!lhs.canHaveFractionalPart() && !rhs.canHaveFractionalPart());
        
        setRange(Range::NewUInt32Range(alloc, 0, UINT32_MAX));
    }
}

void
MSqrt::computeRange(TempAllocator &alloc)
{
    Range input(getOperand(0));

    
    
    if (!input.hasInt32Bounds())
        return;

    
    if (input.lower() < 0)
        return;

    
    
    setRange(new(alloc) Range(0, input.upper(), true, input.exponent()));
}

void
MToDouble::computeRange(TempAllocator &alloc)
{
    setRange(new(alloc) Range(getOperand(0)));
}

void
MToFloat32::computeRange(TempAllocator &alloc)
{
}

void
MTruncateToInt32::computeRange(TempAllocator &alloc)
{
    Range *output = new(alloc) Range(getOperand(0));
    output->wrapAroundToInt32();
    setRange(output);
}

void
MToInt32::computeRange(TempAllocator &alloc)
{
    Range *output = new(alloc) Range(getOperand(0));
    output->clampToInt32();
    setRange(output);
}

void
MLimitedTruncate::computeRange(TempAllocator &alloc)
{
    Range *output = new(alloc) Range(input());
    setRange(output);
}

static Range *GetTypedArrayRange(TempAllocator &alloc, int type)
{
    switch (type) {
      case Scalar::Uint8Clamped:
      case Scalar::Uint8:
        return Range::NewUInt32Range(alloc, 0, UINT8_MAX);
      case Scalar::Uint16:
        return Range::NewUInt32Range(alloc, 0, UINT16_MAX);
      case Scalar::Uint32:
        return Range::NewUInt32Range(alloc, 0, UINT32_MAX);

      case Scalar::Int8:
        return Range::NewInt32Range(alloc, INT8_MIN, INT8_MAX);
      case Scalar::Int16:
        return Range::NewInt32Range(alloc, INT16_MIN, INT16_MAX);
      case Scalar::Int32:
        return Range::NewInt32Range(alloc, INT32_MIN, INT32_MAX);

      case Scalar::Float32:
      case Scalar::Float64:
        break;
    }

  return nullptr;
}

void
MLoadTypedArrayElement::computeRange(TempAllocator &alloc)
{
    
    
    setRange(GetTypedArrayRange(alloc, arrayType()));
}

void
MLoadTypedArrayElementStatic::computeRange(TempAllocator &alloc)
{
    
    
    JS_ASSERT(typedArray_->type() != Scalar::Uint32);

    setRange(GetTypedArrayRange(alloc, typedArray_->type()));
}

void
MArrayLength::computeRange(TempAllocator &alloc)
{
    
    
    
    setRange(Range::NewUInt32Range(alloc, 0, INT32_MAX));
}

void
MInitializedLength::computeRange(TempAllocator &alloc)
{
    setRange(Range::NewUInt32Range(alloc, 0, JSObject::NELEMENTS_LIMIT));
}

void
MTypedArrayLength::computeRange(TempAllocator &alloc)
{
    setRange(Range::NewUInt32Range(alloc, 0, INT32_MAX));
}

void
MStringLength::computeRange(TempAllocator &alloc)
{
    static_assert(JSString::MAX_LENGTH <= UINT32_MAX,
                  "NewUInt32Range requires a uint32 value");
    setRange(Range::NewUInt32Range(alloc, 0, JSString::MAX_LENGTH));
}

void
MArgumentsLength::computeRange(TempAllocator &alloc)
{
    
    
    MOZ_ASSERT(js_JitOptions.maxStackArgs <= UINT32_MAX,
               "NewUInt32Range requires a uint32 value");
    setRange(Range::NewUInt32Range(alloc, 0, js_JitOptions.maxStackArgs));
}

void
MBoundsCheck::computeRange(TempAllocator &alloc)
{
    
    
    
    setRange(new(alloc) Range(index()));
}

void
MArrayPush::computeRange(TempAllocator &alloc)
{
    
    setRange(Range::NewUInt32Range(alloc, 0, UINT32_MAX));
}

void
MMathFunction::computeRange(TempAllocator &alloc)
{
    Range opRange(getOperand(0));
    switch (function()) {
      case Sin:
      case Cos:
        if (!opRange.canBeInfiniteOrNaN())
            setRange(Range::NewDoubleRange(alloc, -1.0, 1.0));
        break;
      case Sign:
        if (!opRange.canBeNaN()) {
            
            int32_t lower = -1;
            int32_t upper = 1;
            if (opRange.hasInt32LowerBound() && opRange.lower() >= 0)
                lower = 0;
            if (opRange.hasInt32UpperBound() && opRange.upper() <= 0)
                upper = 0;
            setRange(Range::NewInt32Range(alloc, lower, upper));
        }
        break;
    default:
        break;
    }
}

void
MRandom::computeRange(TempAllocator &alloc)
{
    setRange(Range::NewDoubleRange(alloc, 0.0, 1.0));
}





bool
RangeAnalysis::analyzeLoop(MBasicBlock *header)
{
    JS_ASSERT(header->hasUniqueBackedge());

    
    
    
    MBasicBlock *backedge = header->backedge();

    
    if (backedge == header)
        return true;

    bool canOsr;
    MarkLoopBlocks(graph_, header, &canOsr);

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
        UnmarkLoopBlocks(graph_, header);
        return true;
    }

    if (!loopIterationBounds.append(iterationBound))
        return false;

#ifdef DEBUG
    if (JitSpewEnabled(JitSpew_Range)) {
        Sprinter sp(GetIonContext()->cx);
        sp.init();
        iterationBound->boundSum.print(sp);
        JitSpew(JitSpew_Range, "computed symbolic bound on backedges: %s",
                sp.string());
    }
#endif

    
    

    for (MPhiIterator iter(header->phisBegin()); iter != header->phisEnd(); iter++)
        analyzeLoopPhi(header, iterationBound, *iter);

    if (!mir->compilingAsmJS()) {
        

        Vector<MBoundsCheck *, 0, IonAllocPolicy> hoistedChecks(alloc());

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

    UnmarkLoopBlocks(graph_, header);
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

    
    
    
    MDefinition *lhsInitial = lhs.term->toPhi()->getLoopPredecessorOperand();
    if (lhsInitial->block()->isMarked())
        return nullptr;

    
    
    MDefinition *lhsWrite = lhs.term->toPhi()->getLoopBackedgeOperand();
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

    LinearSum iterationBound(alloc());
    LinearSum currentIteration(alloc());

    if (lhsModified.constant == 1 && !lessEqual) {
        
        
        
        
        
        

        if (rhs) {
            if (!iterationBound.add(rhs, 1))
                return nullptr;
        }
        if (!iterationBound.add(lhsInitial, -1))
            return nullptr;

        int32_t lhsConstant;
        if (!SafeSub(0, lhs.constant, &lhsConstant))
            return nullptr;
        if (!iterationBound.add(lhsConstant))
            return nullptr;

        if (!currentIteration.add(lhs.term, 1))
            return nullptr;
        if (!currentIteration.add(lhsInitial, -1))
            return nullptr;
    } else if (lhsModified.constant == -1 && lessEqual) {
        
        
        
        
        

        if (!iterationBound.add(lhsInitial, 1))
            return nullptr;
        if (rhs) {
            if (!iterationBound.add(rhs, -1))
                return nullptr;
        }
        if (!iterationBound.add(lhs.constant))
            return nullptr;

        if (!currentIteration.add(lhsInitial, 1))
            return nullptr;
        if (!currentIteration.add(lhs.term, -1))
            return nullptr;
    } else {
        return nullptr;
    }

    return new(alloc()) LoopIterationBound(header, test, iterationBound, currentIteration);
}

void
RangeAnalysis::analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi)
{
    
    
    
    
    
    

    JS_ASSERT(phi->numOperands() == 2);

    MDefinition *initial = phi->getLoopPredecessorOperand();
    if (initial->block()->isMarked())
        return;

    SimpleLinearSum modified = ExtractLinearSum(phi->getLoopBackedgeOperand());

    if (modified.term != phi || modified.constant == 0)
        return;

    if (!phi->range())
        phi->setRange(new(alloc()) Range());

    LinearSum initialSum(alloc());
    if (!initialSum.add(initial, 1))
        return;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    LinearSum limitSum(loopBound->boundSum);
    if (!limitSum.multiply(modified.constant) || !limitSum.add(initialSum))
        return;

    int32_t negativeConstant;
    if (!SafeSub(0, modified.constant, &negativeConstant) || !limitSum.add(negativeConstant))
        return;

    Range *initRange = initial->range();
    if (modified.constant > 0) {
        if (initRange && initRange->hasInt32LowerBound())
            phi->range()->refineLower(initRange->lower());
        phi->range()->setSymbolicLower(SymbolicBound::New(alloc(), nullptr, initialSum));
        phi->range()->setSymbolicUpper(SymbolicBound::New(alloc(), loopBound, limitSum));
    } else {
        if (initRange && initRange->hasInt32UpperBound())
            phi->range()->refineUpper(initRange->upper());
        phi->range()->setSymbolicUpper(SymbolicBound::New(alloc(), nullptr, initialSum));
        phi->range()->setSymbolicLower(SymbolicBound::New(alloc(), loopBound, limitSum));
    }

    JitSpew(JitSpew_Range, "added symbolic range on %d", phi->id());
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

    MDefinition *lowerTerm = ConvertLinearSum(alloc(), preLoop, lower->sum);
    if (!lowerTerm)
        return false;

    MDefinition *upperTerm = ConvertLinearSum(alloc(), preLoop, upper->sum);
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

    MBoundsCheckLower *lowerCheck = MBoundsCheckLower::New(alloc(), lowerTerm);
    lowerCheck->setMinimum(lowerConstant);
    lowerCheck->computeRange(alloc());
    lowerCheck->collectRangeInfoPreTrunc();

    MBoundsCheck *upperCheck = MBoundsCheck::New(alloc(), upperTerm, ins->length());
    upperCheck->setMinimum(upperConstant);
    upperCheck->setMaximum(upperConstant);
    upperCheck->computeRange(alloc());
    upperCheck->collectRangeInfoPreTrunc();

    
    preLoop->insertBefore(preLoop->lastIns(), lowerCheck);
    preLoop->insertBefore(preLoop->lastIns(), upperCheck);

    return true;
}

bool
RangeAnalysis::analyze()
{
    JitSpew(JitSpew_Range, "Doing range propagation");

    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;
        JS_ASSERT(!block->unreachable());

        
        
        
        if (block->immediateDominator()->unreachable()) {
            block->setUnreachable();
            continue;
        }

        for (MDefinitionIterator iter(block); iter; iter++) {
            MDefinition *def = *iter;

            def->computeRange(alloc());
            JitSpew(JitSpew_Range, "computing range on %d", def->id());
            SpewRange(def);
        }

        
        
        if (block->unreachable())
            continue;

        if (block->isLoopHeader()) {
            if (!analyzeLoop(block))
                return false;
        }

        
        
        for (MInstructionIterator iter(block->begin()); iter != block->end(); iter++) {
            iter->collectRangeInfoPreTrunc();

            
            
            if (mir->compilingAsmJS()) {
                uint32_t minHeapLength = mir->minAsmJSHeapLength();
                if (iter->isAsmJSLoadHeap()) {
                    MAsmJSLoadHeap *ins = iter->toAsmJSLoadHeap();
                    Range *range = ins->ptr()->range();
                    if (range && range->hasInt32LowerBound() && range->lower() >= 0 &&
                        range->hasInt32UpperBound() && (uint32_t) range->upper() < minHeapLength) {
                        ins->setSkipBoundsCheck(true);
                    }
                } else if (iter->isAsmJSStoreHeap()) {
                    MAsmJSStoreHeap *ins = iter->toAsmJSStoreHeap();
                    Range *range = ins->ptr()->range();
                    if (range && range->hasInt32LowerBound() && range->lower() >= 0 &&
                        range->hasInt32UpperBound() && (uint32_t) range->upper() < minHeapLength) {
                        ins->setSkipBoundsCheck(true);
                    }
                }
            }
        }
    }

    return true;
}

bool
RangeAnalysis::addRangeAssertions()
{
    if (!js_JitOptions.checkRangeAnalysis)
        return true;

    
    
    
    
    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;

        for (MDefinitionIterator iter(block); iter; iter++) {
            MDefinition *ins = *iter;

            
            if (!IsNumberType(ins->type()) &&
                ins->type() != MIRType_Boolean &&
                ins->type() != MIRType_Value)
            {
                continue;
            }

            Range r(ins);

            
            if (r.isUnknown() || (ins->type() == MIRType_Int32 && r.isUnknownInt32()))
                continue;

            MAssertRange *guard = MAssertRange::New(alloc(), ins, new(alloc()) Range(r));

            
            
            
            MInstructionIterator insertIter = ins->isPhi()
                                            ? block->begin()
                                            : block->begin(ins->toInstruction());
            while (insertIter->isBeta() ||
                   insertIter->isInterruptCheck() ||
                   insertIter->isInterruptCheckPar() ||
                   insertIter->isConstant())
            {
                insertIter++;
            }

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
    if (!hasInt32Bounds()) {
        setInt32(JSVAL_INT_MIN, JSVAL_INT_MAX);
    } else if (canHaveFractionalPart()) {
        canHaveFractionalPart_ = false;

        
        
        refineInt32BoundsByExponent(max_exponent_,
                                    &lower_, &hasInt32LowerBound_,
                                    &upper_, &hasInt32UpperBound_);

        assertInvariants();
    }
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
MDefinition::truncate(TruncateKind kind)
{
    
    return false;
}

bool
MConstant::truncate(TruncateKind kind)
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
MPhi::truncate(TruncateKind kind)
{
    if (type() == MIRType_Double || type() == MIRType_Int32) {
        truncateKind_ = kind;
        setResultType(MIRType_Int32);
        if (kind >= IndirectTruncate && range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MAdd::truncate(TruncateKind kind)
{
    
    setTruncateKind(kind);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (kind >= IndirectTruncate && range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MSub::truncate(TruncateKind kind)
{
    
    setTruncateKind(kind);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (kind >= IndirectTruncate && range())
            range()->wrapAroundToInt32();
        return true;
    }

    return false;
}

bool
MMul::truncate(TruncateKind kind)
{
    
    setTruncateKind(kind);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);
        if (kind >= IndirectTruncate) {
            setCanBeNegativeZero(false);
            if (range())
                range()->wrapAroundToInt32();
        }
        return true;
    }

    return false;
}

bool
MDiv::truncate(TruncateKind kind)
{
    setTruncateKind(kind);

    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);

        
        
        if (tryUseUnsignedOperands())
            unsigned_ = true;

        return true;
    }

    
    return false;
}

bool
MMod::truncate(TruncateKind kind)
{
    
    setTruncateKind(kind);

    
    if (type() == MIRType_Double || type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(MIRType_Int32);

        if (tryUseUnsignedOperands())
            unsigned_ = true;

        return true;
    }

    
    return false;
}

bool
MToDouble::truncate(TruncateKind kind)
{
    JS_ASSERT(type() == MIRType_Double);

    setTruncateKind(kind);

    
    
    setResultType(MIRType_Int32);
    if (kind >= IndirectTruncate) {
        if (range())
            range()->wrapAroundToInt32();
    }

    return true;
}

bool
MLoadTypedArrayElementStatic::truncate(TruncateKind kind)
{
    if (kind >= IndirectTruncate)
        setInfallible();
    return false;
}

bool
MLimitedTruncate::truncate(TruncateKind kind)
{
    setTruncateKind(kind);
    setResultType(MIRType_Int32);
    if (kind >= IndirectTruncate && range())
        range()->wrapAroundToInt32();
    return false;
}

MDefinition::TruncateKind
MDefinition::operandTruncateKind(size_t index) const
{
    
    return NoTruncate;
}

MDefinition::TruncateKind
MPhi::operandTruncateKind(size_t index) const
{
    
    
    return truncateKind_;
}

MDefinition::TruncateKind
MTruncateToInt32::operandTruncateKind(size_t index) const
{
    
    return Truncate;
}

MDefinition::TruncateKind
MBinaryBitwiseInstruction::operandTruncateKind(size_t index) const
{
    
    return Truncate;
}

MDefinition::TruncateKind
MLimitedTruncate::operandTruncateKind(size_t index) const
{
    return Min(truncateKind(), truncateLimit_);
}

MDefinition::TruncateKind
MAdd::operandTruncateKind(size_t index) const
{
    
    
    return Min(truncateKind(), IndirectTruncate);
}

MDefinition::TruncateKind
MSub::operandTruncateKind(size_t index) const
{
    
    return Min(truncateKind(), IndirectTruncate);
}

MDefinition::TruncateKind
MMul::operandTruncateKind(size_t index) const
{
    
    return Min(truncateKind(), IndirectTruncate);
}

MDefinition::TruncateKind
MToDouble::operandTruncateKind(size_t index) const
{
    
    return truncateKind();
}

MDefinition::TruncateKind
MStoreTypedArrayElement::operandTruncateKind(size_t index) const
{
    
    return index == 2 && !isFloatArray() ? Truncate : NoTruncate;
}

MDefinition::TruncateKind
MStoreTypedArrayElementHole::operandTruncateKind(size_t index) const
{
    
    return index == 3 && !isFloatArray() ? Truncate : NoTruncate;
}

MDefinition::TruncateKind
MStoreTypedArrayElementStatic::operandTruncateKind(size_t index) const
{
    
    return index == 1 && !isFloatArray() ? Truncate : NoTruncate;
}

MDefinition::TruncateKind
MDiv::operandTruncateKind(size_t index) const
{
    return Min(truncateKind(), TruncateAfterBailouts);
}

MDefinition::TruncateKind
MMod::operandTruncateKind(size_t index) const
{
    return Min(truncateKind(), TruncateAfterBailouts);
}

bool
MCompare::truncate(TruncateKind kind)
{
    
    
    
    
    if (block()->info().compilingAsmJS())
       return false;

    if (!isDoubleComparison())
        return false;

    
    
    if (!Range(lhs()).isInt32() || !Range(rhs()).isInt32())
        return false;

    compareType_ = Compare_Int32;

    
    
    
    truncateOperands_ = true;

    return true;
}

MDefinition::TruncateKind
MCompare::operandTruncateKind(size_t index) const
{
    
    
    JS_ASSERT_IF(truncateOperands_, isInt32Comparison());
    return truncateOperands_ ? TruncateAfterBailouts : NoTruncate;
}

static void
TruncateTest(TempAllocator &alloc, MTest *test)
{
    
    

    if (test->input()->type() != MIRType_Value)
        return;

    if (!test->input()->isPhi() || !test->input()->hasOneDefUse() || test->input()->isImplicitlyUsed())
        return;

    MPhi *phi = test->input()->toPhi();
    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *def = phi->getOperand(i);
        if (!def->isBox())
            return;
        MDefinition *inner = def->getOperand(0);
        if (inner->type() != MIRType_Boolean && inner->type() != MIRType_Int32)
            return;
    }

    for (size_t i = 0; i < phi->numOperands(); i++) {
        MDefinition *inner = phi->getOperand(i)->getOperand(0);
        if (inner->type() != MIRType_Int32) {
            MBasicBlock *block = inner->block();
            inner = MToInt32::New(alloc, inner);
            block->insertBefore(block->lastIns(), inner->toInstruction());
        }
        JS_ASSERT(inner->type() == MIRType_Int32);
        phi->replaceOperand(i, inner);
    }

    phi->setResultType(MIRType_Int32);
}



static MDefinition::TruncateKind
ComputeRequestedTruncateKind(MDefinition *candidate)
{
    
    
    
    bool needsConversion = !candidate->range() || !candidate->range()->isInt32();

    MDefinition::TruncateKind kind = MDefinition::Truncate;
    for (MUseIterator use(candidate->usesBegin()); use != candidate->usesEnd(); use++) {
        if (!use->consumer()->isDefinition()) {
            
            
            
            
            
            if (candidate->isUseRemoved() && needsConversion)
                kind = Min(kind, MDefinition::TruncateAfterBailouts);
            continue;
        }

        MDefinition *consumer = use->consumer()->toDefinition();
        MDefinition::TruncateKind consumerKind = consumer->operandTruncateKind(consumer->indexOf(*use));
        kind = Min(kind, consumerKind);
        if (kind == MDefinition::NoTruncate)
            break;
    }

    return kind;
}

static MDefinition::TruncateKind
ComputeTruncateKind(MDefinition *candidate)
{
    
    
    if (candidate->isCompare())
        return MDefinition::TruncateAfterBailouts;

    
    
    
    
    const Range *r = candidate->range();
    bool canHaveRoundingErrors = !r || r->canHaveRoundingErrors();

    
    
    if (candidate->isDiv() && candidate->toDiv()->specialization() == MIRType_Int32)
        canHaveRoundingErrors = false;

    if (canHaveRoundingErrors)
        return MDefinition::NoTruncate;

    
    return ComputeRequestedTruncateKind(candidate);
}

static void
RemoveTruncatesOnOutput(MDefinition *truncated)
{
    
    if (truncated->isCompare())
        return;

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
AdjustTruncatedInputs(TempAllocator &alloc, MDefinition *truncated)
{
    MBasicBlock *block = truncated->block();
    for (size_t i = 0, e = truncated->numOperands(); i < e; i++) {
        MDefinition::TruncateKind kind = truncated->operandTruncateKind(i);
        if (kind == MDefinition::NoTruncate)
            continue;

        MDefinition *input = truncated->getOperand(i);
        if (input->type() == MIRType_Int32)
            continue;

        if (input->isToDouble() && input->getOperand(0)->type() == MIRType_Int32) {
            JS_ASSERT(input->range()->isInt32());
            truncated->replaceOperand(i, input->getOperand(0));
        } else {
            MInstruction *op;
            if (kind == MDefinition::TruncateAfterBailouts)
                op = MToInt32::New(alloc, truncated->getOperand(i));
            else
                op = MTruncateToInt32::New(alloc, truncated->getOperand(i));

            if (truncated->isPhi()) {
                MBasicBlock *pred = block->getPredecessor(i);
                pred->insertBefore(pred->lastIns(), op);
            } else {
                block->insertBefore(truncated->toInstruction(), op);
            }
            truncated->replaceOperand(i, op);
        }
    }

    if (truncated->isToDouble()) {
        truncated->replaceAllUsesWith(truncated->toToDouble()->getOperand(0));
        block->discard(truncated->toToDouble());
    }
}












bool
RangeAnalysis::truncate()
{
    JitSpew(JitSpew_Range, "Do range-base truncation (backward loop)");

    
    
    
    
    MOZ_ASSERT(!mir->compilingAsmJS());

    Vector<MDefinition *, 16, SystemAllocPolicy> worklist;
    Vector<MBinaryBitwiseInstruction *, 16, SystemAllocPolicy> bitops;

    for (PostorderIterator block(graph_.poBegin()); block != graph_.poEnd(); block++) {
        for (MInstructionReverseIterator iter(block->rbegin()); iter != block->rend(); iter++) {
            if (iter->type() == MIRType_None) {
                if (iter->isTest())
                    TruncateTest(alloc(), iter->toTest());
                continue;
            }

            
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

            MDefinition::TruncateKind kind = ComputeTruncateKind(*iter);
            if (kind == MDefinition::NoTruncate)
                continue;

            
            if (!iter->truncate(kind))
                continue;

            
            
            iter->setInWorklist();
            if (!worklist.append(*iter))
                return false;
        }
        for (MPhiIterator iter(block->phisBegin()), end(block->phisEnd()); iter != end; ++iter) {
            MDefinition::TruncateKind kind = ComputeTruncateKind(*iter);
            if (kind == MDefinition::NoTruncate)
                continue;

            
            if (!iter->truncate(kind))
                continue;

            
            
            iter->setInWorklist();
            if (!worklist.append(*iter))
                return false;
        }
    }

    
    JitSpew(JitSpew_Range, "Do graph type fixup (dequeue)");
    while (!worklist.empty()) {
        MDefinition *def = worklist.popCopy();
        def->setNotInWorklist();
        RemoveTruncatesOnOutput(def);
        AdjustTruncatedInputs(alloc(), def);
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
MInArray::collectRangeInfoPreTrunc()
{
    Range indexRange(index());
    if (indexRange.isFiniteNonNegative())
        needsNegativeIntCheck_ = false;
}

void
MLoadElementHole::collectRangeInfoPreTrunc()
{
    Range indexRange(index());
    if (indexRange.isFiniteNonNegative())
        needsNegativeIntCheck_ = false;
}

void
MClz::collectRangeInfoPreTrunc()
{
    Range inputRange(input());
    if (!inputRange.canBeZero())
        operandIsNeverZero_ = true;
}

void
MDiv::collectRangeInfoPreTrunc()
{
    Range lhsRange(lhs());
    Range rhsRange(rhs());

    
    if (lhsRange.isFiniteNonNegative())
        canBeNegativeDividend_ = false;

    
    if (!rhsRange.canBeZero())
        canBeDivideByZero_ = false;

    
    
    if (!lhsRange.contains(INT32_MIN))
        canBeNegativeOverflow_ = false;

    
    if (!rhsRange.contains(-1))
        canBeNegativeOverflow_ = false;

    
    
    if (!lhsRange.canBeZero())
        canBeNegativeZero_ = false;

    
    if (rhsRange.isFiniteNonNegative())
        canBeNegativeZero_ = false;
}

void
MMul::collectRangeInfoPreTrunc()
{
    Range lhsRange(lhs());
    Range rhsRange(rhs());

    
    if (lhsRange.isFiniteNonNegative() && !lhsRange.canBeZero())
        setCanBeNegativeZero(false);

    
    if (rhsRange.isFiniteNonNegative() && !rhsRange.canBeZero())
        setCanBeNegativeZero(false);

    
    
    if (rhsRange.isFiniteNonNegative() && lhsRange.isFiniteNonNegative())
        setCanBeNegativeZero(false);

    
    if (rhsRange.isFiniteNegative() && lhsRange.isFiniteNegative())
        setCanBeNegativeZero(false);
}

void
MMod::collectRangeInfoPreTrunc()
{
    Range lhsRange(lhs());
    Range rhsRange(rhs());
    if (lhsRange.isFiniteNonNegative())
        canBeNegativeDividend_ = false;
    if (!rhsRange.canBeZero())
        canBeDivideByZero_ = false;

}

void
MToInt32::collectRangeInfoPreTrunc()
{
    Range inputRange(input());
    if (!inputRange.canBeZero())
        canBeNegativeZero_ = false;
}

void
MBoundsCheckLower::collectRangeInfoPreTrunc()
{
    Range indexRange(index());
    if (indexRange.hasInt32LowerBound() && indexRange.lower() >= minimum_)
        fallible_ = false;
}

void
MCompare::collectRangeInfoPreTrunc()
{
    if (!Range(lhs()).canBeNaN() && !Range(rhs()).canBeNaN())
        operandsAreNeverNaN_ = true;
}

void
MNot::collectRangeInfoPreTrunc()
{
    if (!Range(input()).canBeNaN())
        operandIsNeverNaN_ = true;
}

void
MPowHalf::collectRangeInfoPreTrunc()
{
    Range inputRange(input());
    if (!inputRange.canBeInfiniteOrNaN() || inputRange.hasInt32LowerBound())
        operandIsNeverNegativeInfinity_ = true;
    if (!inputRange.canBeZero())
        operandIsNeverNegativeZero_ = true;
    if (!inputRange.canBeNaN())
        operandIsNeverNaN_ = true;
}

void
MUrsh::collectRangeInfoPreTrunc()
{
    Range lhsRange(lhs()), rhsRange(rhs());

    
    lhsRange.wrapAroundToInt32();
    rhsRange.wrapAroundToShiftCount();

    
    
    if (lhsRange.lower() >= 0 || rhsRange.lower() >= 1)
        bailoutsDisabled_ = true;
}

bool
RangeAnalysis::prepareForUCE(bool *shouldRemoveDeadCode)
{
    *shouldRemoveDeadCode = false;

    for (ReversePostorderIterator iter(graph_.rpoBegin()); iter != graph_.rpoEnd(); iter++) {
        MBasicBlock *block = *iter;

        if (!block->unreachable())
            continue;

        MControlInstruction *cond = block->getPredecessor(0)->lastIns();
        if (!cond->isTest())
            continue;

        
        
        
        MTest *test = cond->toTest();
        MConstant *constant = nullptr;
        if (block == test->ifTrue()) {
            constant = MConstant::New(alloc(), BooleanValue(false));
        } else {
            JS_ASSERT(block == test->ifFalse());
            constant = MConstant::New(alloc(), BooleanValue(true));
        }
        test->block()->insertBefore(test, constant);
        test->replaceOperand(0, constant);
        JitSpew(JitSpew_Range, "Update condition of %d to reflect unreachable branches.",
                test->id());

        *shouldRemoveDeadCode = true;
    }

    return true;
}
