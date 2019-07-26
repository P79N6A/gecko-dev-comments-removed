






#ifndef jsion_range_analysis_h__
#define jsion_range_analysis_h__

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include "wtf/Platform.h"
#include "MIR.h"
#include "CompileInfo.h"
#include "IonAnalysis.h"

namespace js {
namespace ion {

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
    MIRGraph &graph_;

  public:
    RangeAnalysis(MIRGraph &graph);
    bool addBetaNobes();
    bool analyze();
    bool removeBetaNobes();
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

    
    
    
    static const uint16_t MaxTruncatableExponent = MOZ_DOUBLE_EXPONENT_SHIFT;

    
    static const uint16_t MaxDoubleExponent = MOZ_DOUBLE_EXPONENT_BIAS;

  private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    int32_t lower_;
    bool lower_infinite_;

    int32_t upper_;
    bool upper_infinite_;

    bool decimal_;
    uint16_t max_exponent_;

    
    const SymbolicBound *symbolicLower_;
    const SymbolicBound *symbolicUpper_;

  public:
    Range()
        : lower_(JSVAL_INT_MIN),
          lower_infinite_(true),
          upper_(JSVAL_INT_MAX),
          upper_infinite_(true),
          decimal_(true),
          max_exponent_(MaxDoubleExponent),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    Range(int64_t l, int64_t h, bool d = false, uint16_t e = MaxInt32Exponent)
        : lower_infinite_(true),
          upper_infinite_(true),
          decimal_(d),
          max_exponent_(e),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
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
          decimal_(other.decimal_),
          max_exponent_(other.max_exponent_),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    Range(const MDefinition *def);

    static Range *Truncate(int64_t l, int64_t h);

    void print(Sprinter &sp) const;
    bool update(const Range *other);
    bool update(const Range &other) {
        return update(&other);
    }

    
    
    
    
    void unionWith(const Range *other);
    static Range * intersect(const Range *lhs, const Range *rhs, bool *emptyRange);
    static Range * add(const Range *lhs, const Range *rhs);
    static Range * sub(const Range *lhs, const Range *rhs);
    static Range * mul(const Range *lhs, const Range *rhs);
    static Range * and_(const Range *lhs, const Range *rhs);
    static Range * shl(const Range *lhs, int32_t c);
    static Range * shr(const Range *lhs, int32_t c);

    static bool negativeZeroMul(const Range *lhs, const Range *rhs);

    inline void makeLowerInfinite() {
        lower_infinite_ = true;
        lower_ = JSVAL_INT_MIN;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
    }
    inline void makeUpperInfinite() {
        upper_infinite_ = true;
        upper_ = JSVAL_INT_MAX;
        if (max_exponent_ < MaxInt32Exponent)
            max_exponent_ = MaxInt32Exponent;
    }
    inline void makeRangeInfinite() {
        makeLowerInfinite();
        makeUpperInfinite();
        max_exponent_ = MaxDoubleExponent;
    }

    inline bool isLowerInfinite() const {
        return lower_infinite_;
    }
    inline bool isUpperInfinite() const {
        return upper_infinite_;
    }

    inline bool isInt32() const {
        return !isLowerInfinite() && !isUpperInfinite();
    }

    inline bool hasRoundingErrors() const {
        return isDecimal() || exponent() >= MaxTruncatableExponent;
    }

    inline bool isInfinite() const {
        return exponent() >= MaxDoubleExponent;
    }

    inline bool isDecimal() const {
        return decimal_;
    }

    inline uint16_t exponent() const {
        return max_exponent_;
    }

    inline uint16_t numBits() const {
        return max_exponent_ + 1; 
    }

    inline int32_t lower() const {
        return lower_;
    }

    inline int32_t upper() const {
        return upper_;
    }

    inline void setLowerInit(int64_t x) {
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
    inline void setLower(int64_t x) {
        setLowerInit(x);
        rectifyExponent();
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
    }
    inline void setUpperInit(int64_t x) {
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
    inline void setUpper(int64_t x) {
        setUpperInit(x);
        rectifyExponent();
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    inline void setInt32() {
        lower_infinite_ = false;
        upper_infinite_ = false;
        decimal_ = false;
        max_exponent_ = MaxInt32Exponent;
    }

    inline void set(int64_t l, int64_t h, bool d, uint16_t e) {
        setLowerInit(l);
        setUpperInit(h);
        decimal_ = d;
        max_exponent_ = e;
        rectifyExponent();
        JS_ASSERT_IF(lower_infinite_, lower_ == JSVAL_INT_MIN);
        JS_ASSERT_IF(upper_infinite_, upper_ == JSVAL_INT_MAX);
    }

    
    void truncate();

    
    
    
    
    
    
    
    inline void rectifyExponent() {
        if (!isInt32()) {
            JS_ASSERT(max_exponent_ >= MaxInt32Exponent);
            return;
        }

        uint32_t max = Max(mozilla::Abs<int64_t>(lower()), mozilla::Abs<int64_t>(upper()));
        JS_ASSERT_IF(lower() == JSVAL_INT_MIN, max == (uint32_t) JSVAL_INT_MIN);
        JS_ASSERT(max <= (uint32_t) JSVAL_INT_MIN);
        
        max_exponent_ = max ? js_FloorLog2wImpl(max) : max;
    }

    const SymbolicBound *symbolicLower() const {
        return symbolicLower_;
    }
    const SymbolicBound *symbolicUpper() const {
        return symbolicUpper_;
    }

    inline void setSymbolicLower(SymbolicBound *bound) {
        symbolicLower_ = bound;
    }
    inline void setSymbolicUpper(SymbolicBound *bound) {
        symbolicUpper_ = bound;
    }
};

} 
} 

#endif 

