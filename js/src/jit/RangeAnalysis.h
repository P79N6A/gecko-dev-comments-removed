





#ifndef jit_RangeAnalysis_h
#define jit_RangeAnalysis_h

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include "jit/CompileInfo.h"
#include "jit/IonAnalysis.h"
#include "jit/MIR.h"

namespace js {
namespace jit {

class MBasicBlock;
class MIRGraph;





struct LoopIterationBound : public TempObject
{
    
    MBasicBlock *header;

    
    
    
    
    MTest *test;

    
    LinearSum sum;

    LoopIterationBound(MBasicBlock *header, MTest *test, LinearSum sum)
      : header(header), test(test), sum(sum)
    {
    }
};


struct SymbolicBound : public TempObject
{
    
    
    
    
    
    
    LoopIterationBound *loop;

    
    LinearSum sum;

    SymbolicBound(LoopIterationBound *loop, LinearSum sum)
      : loop(loop), sum(sum)
    {
    }

    void print(Sprinter &sp) const;
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

  public:
    MOZ_CONSTEXPR RangeAnalysis(MIRGenerator *mir, MIRGraph &graph) :
        mir(mir), graph_(graph) {}
    bool addBetaNodes();
    bool analyze();
    bool addRangeAssertions();
    bool removeBetaNodes();
    bool truncate();

  private:
    bool analyzeLoop(MBasicBlock *header);
    LoopIterationBound *analyzeLoopIterationCount(MBasicBlock *header,
                                                  MTest *test, BranchDirection direction);
    void analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi);
    bool tryHoistBoundsCheck(MBasicBlock *header, MBoundsCheck *ins);
    bool markBlocksInLoopBody(MBasicBlock *header, MBasicBlock *current);
};

class Range : public TempObject {
  public:
    
    
    static const uint16_t MaxInt32Exponent = 31;

    
    
    static const uint16_t MaxUInt32Exponent = 31;

    
    
    
    static const uint16_t MaxTruncatableExponent = mozilla::DoubleExponentShift;

    
    static const uint16_t MaxDoubleExponent = mozilla::DoubleExponentBias;

    
    
    
    
    
    
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

    void setLowerInit(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            lower_ = JSVAL_INT_MAX;
            hasInt32LowerBound_ = true;
        } else if (x < JSVAL_INT_MIN) {
            dropInt32LowerBound();
        } else {
            lower_ = int32_t(x);
            hasInt32LowerBound_ = true;
        }
    }
    void setUpperInit(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            dropInt32UpperBound();
        } else if (x < JSVAL_INT_MIN) {
            upper_ = JSVAL_INT_MIN;
            hasInt32UpperBound_ = true;
        } else {
            upper_ = int32_t(x);
            hasInt32UpperBound_ = true;
        }
    }

  public:
    Range()
        : lower_(JSVAL_INT_MIN),
          hasInt32LowerBound_(false),
          upper_(JSVAL_INT_MAX),
          hasInt32UpperBound_(false),
          canHaveFractionalPart_(true),
          max_exponent_(MaxDoubleExponent),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);
    }

    Range(int64_t l, int64_t h, bool f = false, uint16_t e = MaxInt32Exponent)
        : hasInt32LowerBound_(false),
          hasInt32UpperBound_(false),
          canHaveFractionalPart_(f),
          max_exponent_(e),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT(e >= (h == INT64_MIN ? MaxDoubleExponent : mozilla::FloorLog2(mozilla::Abs(h))));
        JS_ASSERT(e >= (l == INT64_MIN ? MaxDoubleExponent : mozilla::FloorLog2(mozilla::Abs(l))));

        setLowerInit(l);
        setUpperInit(h);
        rectifyExponent();
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);
    }

    Range(const Range &other)
        : lower_(other.lower_),
          hasInt32LowerBound_(other.hasInt32LowerBound_),
          upper_(other.upper_),
          hasInt32UpperBound_(other.hasInt32UpperBound_),
          canHaveFractionalPart_(other.canHaveFractionalPart_),
          max_exponent_(other.max_exponent_),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);
    }

    Range(const MDefinition *def);

    static Range *NewInt32Range(int32_t l, int32_t h) {
        return new Range(l, h, false, MaxInt32Exponent);
    }

    static Range *NewUInt32Range(uint32_t l, uint32_t h) {
        
        
        return new Range(l, h, false, MaxUInt32Exponent);
    }

    static Range *NewDoubleRange(int64_t l, int64_t h, uint16_t e = MaxDoubleExponent) {
        return new Range(l, h, true, e);
    }

    static Range *NewSingleValueRange(int64_t v) {
        return new Range(v, v, false, MaxDoubleExponent);
    }

    void print(Sprinter &sp) const;
    bool update(const Range *other);

    
    
    
    
    void unionWith(const Range *other);
    static Range * intersect(const Range *lhs, const Range *rhs, bool *emptyRange);
    static Range * add(const Range *lhs, const Range *rhs);
    static Range * sub(const Range *lhs, const Range *rhs);
    static Range * mul(const Range *lhs, const Range *rhs);
    static Range * and_(const Range *lhs, const Range *rhs);
    static Range * or_(const Range *lhs, const Range *rhs);
    static Range * xor_(const Range *lhs, const Range *rhs);
    static Range * not_(const Range *op);
    static Range * lsh(const Range *lhs, int32_t c);
    static Range * rsh(const Range *lhs, int32_t c);
    static Range * ursh(const Range *lhs, int32_t c);
    static Range * lsh(const Range *lhs, const Range *rhs);
    static Range * rsh(const Range *lhs, const Range *rhs);
    static Range * ursh(const Range *lhs, const Range *rhs);
    static Range * abs(const Range *op);
    static Range * min(const Range *lhs, const Range *rhs);
    static Range * max(const Range *lhs, const Range *rhs);

    static bool negativeZeroMul(const Range *lhs, const Range *rhs);

    void dropInt32LowerBound() {
        hasInt32LowerBound_ = false;
        lower_ = JSVAL_INT_MIN;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
    }
    void dropInt32UpperBound() {
        hasInt32UpperBound_ = false;
        upper_ = JSVAL_INT_MAX;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
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
        return canHaveFractionalPart() || exponent() >= MaxTruncatableExponent;
    }

    bool canBeInfiniteOrNaN() const {
        return exponent() >= MaxDoubleExponent;
    }

    bool canHaveFractionalPart() const {
        return canHaveFractionalPart_;
    }

    uint16_t exponent() const {
        return max_exponent_;
    }

    uint16_t numBits() const {
        return max_exponent_ + 1; 
    }

    int32_t lower() const {
        return lower_;
    }

    int32_t upper() const {
        return upper_;
    }

    void setLower(int64_t x) {
        setLowerInit(x);
        rectifyExponent();
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
    }
    void setUpper(int64_t x) {
        setUpperInit(x);
        rectifyExponent();
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);
    }

    void setInt32() {
        hasInt32LowerBound_ = true;
        hasInt32UpperBound_ = true;
        canHaveFractionalPart_ = false;
        max_exponent_ = MaxInt32Exponent;
    }

    void set(int64_t l, int64_t h, bool f = false, uint16_t e = MaxInt32Exponent) {
        max_exponent_ = e;
        setLowerInit(l);
        setUpperInit(h);
        canHaveFractionalPart_ = f;
        rectifyExponent();
        JS_ASSERT_IF(!hasInt32LowerBound_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(!hasInt32UpperBound_, upper_ == JSVAL_INT_MAX);
    }

    
    
    void clampToInt32();

    
    
    void wrapAroundToInt32();

    
    
    void wrapAroundToShiftCount();

    
    
    void wrapAroundToBoolean();

    
    
    
    void extendUInt32ToInt32Min() {
        JS_ASSERT(!hasInt32UpperBound());
        lower_ = JSVAL_INT_MIN;
    }

    
    
    
    
    
    
    
    void rectifyExponent() {
        if (!hasInt32Bounds()) {
            JS_ASSERT(max_exponent_ >= MaxInt32Exponent);
            return;
        }

        uint32_t max = Max(mozilla::Abs<int64_t>(lower()), mozilla::Abs<int64_t>(upper()));
        JS_ASSERT_IF(lower() == JSVAL_INT_MIN, max == (uint32_t) JSVAL_INT_MIN);
        JS_ASSERT(max <= (uint32_t) JSVAL_INT_MIN);
        
        max_exponent_ = max ? mozilla::FloorLog2Size(max) : max;
    }

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
