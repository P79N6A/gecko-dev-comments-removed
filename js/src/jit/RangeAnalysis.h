





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
    void analyzeLoop(MBasicBlock *header);
    LoopIterationBound *analyzeLoopIterationCount(MBasicBlock *header,
                                                  MTest *test, BranchDirection direction);
    void analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi);
    bool tryHoistBoundsCheck(MBasicBlock *header, MBoundsCheck *ins);
    void markBlocksInLoopBody(MBasicBlock *header, MBasicBlock *current);
};

class Range : public TempObject {
  public:
    
    
    static const uint16_t MaxInt32Exponent = 31;

    
    
    
    static const uint16_t MaxTruncatableExponent = mozilla::DoubleExponentShift;

    
    static const uint16_t MaxDoubleExponent = mozilla::DoubleExponentBias;

  private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    int32_t lower_;
    bool lower_infinite_;

    int32_t upper_;
    bool upper_infinite_;

    bool canHaveFractionalPart_;
    uint16_t max_exponent_;

    
    const SymbolicBound *symbolicLower_;
    const SymbolicBound *symbolicUpper_;

  public:
    Range()
        : lower_(JSVAL_INT_MIN),
          lower_infinite_(true),
          upper_(JSVAL_INT_MAX),
          upper_infinite_(true),
          canHaveFractionalPart_(true),
          max_exponent_(MaxDoubleExponent),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    Range(int64_t l, int64_t h, bool f = false, uint16_t e = MaxInt32Exponent)
        : lower_infinite_(true),
          upper_infinite_(true),
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
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    Range(const Range &other)
        : lower_(other.lower_),
          lower_infinite_(other.lower_infinite_),
          upper_(other.upper_),
          upper_infinite_(other.upper_infinite_),
          canHaveFractionalPart_(other.canHaveFractionalPart_),
          max_exponent_(other.max_exponent_),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    Range(const MDefinition *def);

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

    void makeLowerInfinite() {
        lower_infinite_ = true;
        lower_ = JSVAL_INT_MIN;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
    }
    void makeUpperInfinite() {
        upper_infinite_ = true;
        upper_ = JSVAL_INT_MAX;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
    }

    bool isLowerInfinite() const {
        return lower_infinite_;
    }
    bool isUpperInfinite() const {
        return upper_infinite_;
    }

    bool isInt32() const {
        return !isLowerInfinite() && !isUpperInfinite();
    }
    bool isBoolean() const {
        return lower() >= 0 && upper() <= 1;
    }

    bool hasRoundingErrors() const {
        return canHaveFractionalPart() || exponent() >= MaxTruncatableExponent;
    }

    bool isInfinite() const {
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

    void setLowerInit(int64_t x) {
        if (x > JSVAL_INT_MAX) { 
            lower_ = JSVAL_INT_MAX;
            lower_infinite_ = false;
        } else if (x < JSVAL_INT_MIN) {
            makeLowerInfinite();
        } else {
            lower_ = (int32_t)x;
            lower_infinite_ = false;
        }
    }
    void setLower(int64_t x) {
        setLowerInit(x);
        rectifyExponent();
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
    }
    void setUpperInit(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            makeUpperInfinite();
        } else if (x < JSVAL_INT_MIN) { 
            upper_ = JSVAL_INT_MIN;
            upper_infinite_ = false;
        } else {
            upper_ = (int32_t)x;
            upper_infinite_ = false;
        }
    }
    void setUpper(int64_t x) {
        setUpperInit(x);
        rectifyExponent();
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    void setInt32() {
        lower_infinite_ = false;
        upper_infinite_ = false;
        canHaveFractionalPart_ = false;
        max_exponent_ = MaxInt32Exponent;
    }

    void set(int64_t l, int64_t h, bool f = false, uint16_t e = MaxInt32Exponent) {
        max_exponent_ = e;
        setLowerInit(l);
        setUpperInit(h);
        canHaveFractionalPart_ = f;
        rectifyExponent();
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    
    
    void clampToInt32();

    
    
    void wrapAroundToInt32();

    
    
    void wrapAroundToShiftCount();

    
    
    void wrapAroundToBoolean();

    
    
    
    void extendUInt32ToInt32Min() {
        JS_ASSERT(isUpperInfinite());
        lower_ = JSVAL_INT_MIN;
    }

    
    
    
    
    
    
    
    void rectifyExponent() {
        if (!isInt32()) {
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
