





#ifndef jit_RangeAnalysis_h
#define jit_RangeAnalysis_h

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include "jit/IonAnalysis.h"
#include "jit/MIR.h"


#undef min
#undef max

namespace js {
namespace jit {

class MBasicBlock;
class MIRGraph;





struct LoopIterationBound : public TempObject
{
    
    MBasicBlock *header;

    
    
    
    
    
    MTest *test;

    
    
    LinearSum boundSum;

    
    
    LinearSum currentSum;

    LoopIterationBound(MBasicBlock *header, MTest *test, LinearSum boundSum, LinearSum currentSum)
      : header(header), test(test),
        boundSum(boundSum), currentSum(currentSum)
    {
    }
};

typedef Vector<LoopIterationBound *, 0, SystemAllocPolicy> LoopIterationBoundVector;


struct SymbolicBound : public TempObject
{
  private:
    SymbolicBound(LoopIterationBound *loop, LinearSum sum)
      : loop(loop), sum(sum)
    {
    }

  public:
    
    
    
    
    
    
    LoopIterationBound *loop;

    static SymbolicBound *New(TempAllocator &alloc, LoopIterationBound *loop, LinearSum sum) {
        return new(alloc) SymbolicBound(loop, sum);
    }

    
    LinearSum sum;

    void print(Sprinter &sp) const;
    void dump() const;
};

class RangeAnalysis
{
  protected:
    bool blockDominates(MBasicBlock *b, MBasicBlock *b2);
    void replaceDominatedUsesWith(MDefinition *orig, MDefinition *dom,
                                  MBasicBlock *block);

  protected:
    MIRGenerator *mir;
    MIRGraph &graph_;

    TempAllocator &alloc() const;

  public:
    RangeAnalysis(MIRGenerator *mir, MIRGraph &graph) :
        mir(mir), graph_(graph) {}
    bool addBetaNodes();
    bool analyze();
    bool addRangeAssertions();
    bool removeBetaNodes();
    bool prepareForUCE(bool *shouldRemoveDeadCode);
    bool truncate();

    
    LoopIterationBoundVector loopIterationBounds;

  private:
    bool analyzeLoop(MBasicBlock *header);
    LoopIterationBound *analyzeLoopIterationCount(MBasicBlock *header,
                                                  MTest *test, BranchDirection direction);
    void analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi);
    bool tryHoistBoundsCheck(MBasicBlock *header, MBoundsCheck *ins);
};

class Range : public TempObject {
  public:
    
    
    static const uint16_t MaxInt32Exponent = 31;

    
    
    static const uint16_t MaxUInt32Exponent = 31;

    
    
    
    static const uint16_t MaxTruncatableExponent = mozilla::FloatingPoint<double>::kExponentShift;

    
    static const uint16_t MaxFiniteExponent = mozilla::FloatingPoint<double>::kExponentBias;

    
    
    static const uint16_t IncludesInfinity = MaxFiniteExponent + 1;

    
    
    static const uint16_t IncludesInfinityAndNaN = UINT16_MAX;

    
    
    
    
    
    
    static const int64_t NoInt32UpperBound = int64_t(JSVAL_INT_MAX) + 1;
    static const int64_t NoInt32LowerBound = int64_t(JSVAL_INT_MIN) - 1;

  private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    int32_t lower_;
    bool hasInt32LowerBound_;

    int32_t upper_;
    bool hasInt32UpperBound_;

    bool canHaveFractionalPart_;
    uint16_t max_exponent_;

    
    const SymbolicBound *symbolicLower_;
    const SymbolicBound *symbolicUpper_;

    
    
    void assertInvariants() const {
        
        JS_ASSERT(lower_ <= upper_);

        
        
        
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);

        
        JS_ASSERT(max_exponent_ <= MaxFiniteExponent ||
                  max_exponent_ == IncludesInfinity ||
                  max_exponent_ == IncludesInfinityAndNaN);

        
        
        
        
        
        
        
        
        JS_ASSERT_IF(!hasInt32LowerBound_ || !hasInt32UpperBound_,
                     max_exponent_ + canHaveFractionalPart_ >= MaxInt32Exponent);
        JS_ASSERT(max_exponent_ + canHaveFractionalPart_ >=
                  mozilla::FloorLog2(mozilla::Abs(upper_)));
        JS_ASSERT(max_exponent_ + canHaveFractionalPart_ >=
                  mozilla::FloorLog2(mozilla::Abs(lower_)));

        
        
        JS_ASSERT(mozilla::FloorLog2(JSVAL_INT_MIN) == MaxInt32Exponent);
        JS_ASSERT(mozilla::FloorLog2(JSVAL_INT_MAX) == 30);
        JS_ASSERT(mozilla::FloorLog2(UINT32_MAX) == MaxUInt32Exponent);
        JS_ASSERT(mozilla::FloorLog2(0) == 0);
    }

    
    void setLowerInit(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            lower_ = JSVAL_INT_MAX;
            hasInt32LowerBound_ = true;
        } else if (x < JSVAL_INT_MIN) {
            lower_ = JSVAL_INT_MIN;
            hasInt32LowerBound_ = false;
        } else {
            lower_ = int32_t(x);
            hasInt32LowerBound_ = true;
        }
    }
    
    void setUpperInit(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            upper_ = JSVAL_INT_MAX;
            hasInt32UpperBound_ = false;
        } else if (x < JSVAL_INT_MIN) {
            upper_ = JSVAL_INT_MIN;
            hasInt32UpperBound_ = true;
        } else {
            upper_ = int32_t(x);
            hasInt32UpperBound_ = true;
        }
    }

    
    
    
    
    
    
    uint16_t exponentImpliedByInt32Bounds() const {
         
         uint32_t max = Max(mozilla::Abs(lower()), mozilla::Abs(upper()));
         uint16_t result = mozilla::FloorLog2(max);
         JS_ASSERT(result == (max == 0 ? 0 : mozilla::ExponentComponent(double(max))));
         return result;
    }

    
    
    
    
    
    
    
    
    
    static void refineInt32BoundsByExponent(uint16_t e,
                                            int32_t *l, bool *lb,
                                            int32_t *h, bool *hb)
    {
       if (e < MaxInt32Exponent) {
           
           int32_t limit = (uint32_t(1) << (e + 1)) - 1;
           *h = Min(*h, limit);
           *l = Max(*l, -limit);
           *hb = true;
           *lb = true;
       }
    }

    
    
    
    void optimize() {
        assertInvariants();

        if (hasInt32Bounds()) {
            
            
            
            uint16_t newExponent = exponentImpliedByInt32Bounds();
            if (newExponent < max_exponent_) {
                max_exponent_ = newExponent;
                assertInvariants();
            }

            
            
            if (canHaveFractionalPart_ && lower_ == upper_) {
                canHaveFractionalPart_ = false;
                assertInvariants();
            }
        }
    }

    
    void rawInitialize(int32_t l, bool lb, int32_t h, bool hb, bool f, uint16_t e) {
        lower_ = l;
        hasInt32LowerBound_ = lb;
        upper_ = h;
        hasInt32UpperBound_ = hb;
        canHaveFractionalPart_ = f;
        max_exponent_ = e;
        optimize();
    }

    
    Range(int32_t l, bool lb, int32_t h, bool hb, bool f, uint16_t e)
      : symbolicLower_(nullptr),
        symbolicUpper_(nullptr)
     {
        rawInitialize(l, lb, h, hb, f, e);
     }

  public:
    Range()
      : symbolicLower_(nullptr),
        symbolicUpper_(nullptr)
    {
        setUnknown();
    }

    Range(int64_t l, int64_t h, bool f = false, uint16_t e = MaxInt32Exponent)
      : symbolicLower_(nullptr),
        symbolicUpper_(nullptr)
    {
        set(l, h, f, e);
    }

    Range(const Range &other)
      : lower_(other.lower_),
        hasInt32LowerBound_(other.hasInt32LowerBound_),
        upper_(other.upper_),
        hasInt32UpperBound_(other.hasInt32UpperBound_),
        canHaveFractionalPart_(other.canHaveFractionalPart_),
        max_exponent_(other.max_exponent_),
        symbolicLower_(nullptr),
        symbolicUpper_(nullptr)
    {
        assertInvariants();
    }

    
    
    
    explicit Range(const MDefinition *def);

    static Range *NewInt32Range(TempAllocator &alloc, int32_t l, int32_t h) {
        return new(alloc) Range(l, h, false, MaxInt32Exponent);
    }

    static Range *NewUInt32Range(TempAllocator &alloc, uint32_t l, uint32_t h) {
        
        
        return new(alloc) Range(l, h, false, MaxUInt32Exponent);
    }

    static Range *NewDoubleRange(TempAllocator &alloc, double l, double h) {
        if (mozilla::IsNaN(l) && mozilla::IsNaN(h))
            return nullptr;

        Range *r = new(alloc) Range();
        r->setDouble(l, h);
        return r;
    }

    void print(Sprinter &sp) const;
    void dump(FILE *fp) const;
    void dump() const;
    bool update(const Range *other);

    
    
    
    
    void unionWith(const Range *other);
    static Range *intersect(TempAllocator &alloc, const Range *lhs, const Range *rhs,
                             bool *emptyRange);
    static Range *add(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *sub(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *mul(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *and_(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *or_(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *xor_(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *not_(TempAllocator &alloc, const Range *op);
    static Range *lsh(TempAllocator &alloc, const Range *lhs, int32_t c);
    static Range *rsh(TempAllocator &alloc, const Range *lhs, int32_t c);
    static Range *ursh(TempAllocator &alloc, const Range *lhs, int32_t c);
    static Range *lsh(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *rsh(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *ursh(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *abs(TempAllocator &alloc, const Range *op);
    static Range *min(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *max(TempAllocator &alloc, const Range *lhs, const Range *rhs);
    static Range *floor(TempAllocator &alloc, const Range *op);
    static Range *ceil(TempAllocator &alloc, const Range *op);

    static bool negativeZeroMul(const Range *lhs, const Range *rhs);

    bool isUnknownInt32() const {
        return isInt32() && lower() == INT32_MIN && upper() == INT32_MAX;
    }

    bool isUnknown() const {
        return !hasInt32LowerBound_ &&
               !hasInt32UpperBound_ &&
               canHaveFractionalPart_ &&
               max_exponent_ == IncludesInfinityAndNaN;
    }

    bool hasInt32LowerBound() const {
        return hasInt32LowerBound_;
    }
    bool hasInt32UpperBound() const {
        return hasInt32UpperBound_;
    }

    
    
    bool hasInt32Bounds() const {
        return hasInt32LowerBound() && hasInt32UpperBound();
    }

    
    bool isInt32() const {
        return hasInt32Bounds() && !canHaveFractionalPart();
    }

    
    bool isBoolean() const {
        return lower() >= 0 && upper() <= 1 && !canHaveFractionalPart();
    }

    bool canHaveRoundingErrors() const {
        return canHaveFractionalPart() || max_exponent_ >= MaxTruncatableExponent;
    }

    
    bool contains(int32_t x) const {
        return x >= lower_ && x <= upper_;
    }

    
    bool canBeZero() const {
        return contains(0);
    }

    
    bool canBeNaN() const {
        return max_exponent_ == IncludesInfinityAndNaN;
    }

    
    bool canBeInfiniteOrNaN() const {
        return max_exponent_ >= IncludesInfinity;
    }

    bool canHaveFractionalPart() const {
        return canHaveFractionalPart_;
    }

    uint16_t exponent() const {
        JS_ASSERT(!canBeInfiniteOrNaN());
        return max_exponent_;
    }

    uint16_t numBits() const {
        return exponent() + 1; 
    }

    
    int32_t lower() const {
        JS_ASSERT(hasInt32LowerBound());
        return lower_;
    }

    
    int32_t upper() const {
        JS_ASSERT(hasInt32UpperBound());
        return upper_;
    }

    
    bool isFiniteNegative() const {
        return upper_ < 0 && !canBeInfiniteOrNaN();
    }

    
    bool isFiniteNonNegative() const {
        return lower_ >= 0 && !canBeInfiniteOrNaN();
    }

    
    
    bool canBeFiniteNegative() const {
        return lower_ < 0;
    }

    
    
    bool canBeFiniteNonNegative() const {
        return upper_ >= 0;
    }

    
    void refineLower(int32_t x) {
        assertInvariants();
        hasInt32LowerBound_ = true;
        lower_ = Max(lower_, x);
        optimize();
    }

    
    void refineUpper(int32_t x) {
        assertInvariants();
        hasInt32UpperBound_ = true;
        upper_ = Min(upper_, x);
        optimize();
    }

    void setInt32(int32_t l, int32_t h) {
        hasInt32LowerBound_ = true;
        hasInt32UpperBound_ = true;
        lower_ = l;
        upper_ = h;
        canHaveFractionalPart_ = false;
        max_exponent_ = exponentImpliedByInt32Bounds();
        assertInvariants();
    }

    void setDouble(double l, double h);

    void setUnknown() {
        set(NoInt32LowerBound, NoInt32UpperBound, true, IncludesInfinityAndNaN);
        JS_ASSERT(isUnknown());
    }

    void set(int64_t l, int64_t h, bool f, uint16_t e) {
        max_exponent_ = e;
        canHaveFractionalPart_ = f;
        setLowerInit(l);
        setUpperInit(h);
        optimize();
    }

    
    
    void clampToInt32();

    
    
    void wrapAroundToInt32();

    
    
    void wrapAroundToShiftCount();

    
    
    void wrapAroundToBoolean();

    const SymbolicBound *symbolicLower() const {
        return symbolicLower_;
    }
    const SymbolicBound *symbolicUpper() const {
        return symbolicUpper_;
    }

    void setSymbolicLower(SymbolicBound *bound) {
        symbolicLower_ = bound;
    }
    void setSymbolicUpper(SymbolicBound *bound) {
        symbolicUpper_ = bound;
    }
};

} 
} 

#endif 
