






#ifndef jsion_range_analysis_h__
#define jsion_range_analysis_h__

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

  private:
    void analyzeLoop(MBasicBlock *header);
    LoopIterationBound *analyzeLoopIterationCount(MBasicBlock *header,
                                                  MTest *test, BranchDirection direction);
    void analyzeLoopPhi(MBasicBlock *header, LoopIterationBound *loopBound, MPhi *phi);
    bool tryHoistBoundsCheck(MBasicBlock *header, MBoundsCheck *ins);
    void markBlocksInLoopBody(MBasicBlock *header, MBasicBlock *current);
};

class Range : public TempObject {
  private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t lower_;
    bool lower_infinite_;
    int32_t upper_;
    bool upper_infinite_;

    
    const SymbolicBound *symbolicLower_;
    const SymbolicBound *symbolicUpper_;

  public:
    Range()
        : lower_(JSVAL_INT_MIN),
          lower_infinite_(true),
          upper_(JSVAL_INT_MAX),
          upper_infinite_(true),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {}

    Range(int64_t l, int64_t h)
        : symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {
        setLower(l);
        setUpper(h);
    }

    Range(const Range &other)
        : lower_(other.lower_),
          lower_infinite_(other.lower_infinite_),
          upper_(other.upper_),
          upper_infinite_(other.upper_infinite_),
          symbolicLower_(NULL),
          symbolicUpper_(NULL)
    {}

    static Range *Truncate(int64_t l, int64_t h);

    static int64_t abs64(int64_t x) {
#ifdef WTF_OS_WINDOWS
        return _abs64(x);
#else
        return llabs(x);
#endif
    }

    void print(Sprinter &sp) const;
    bool update(const Range *other);
    bool update(const Range &other) {
        return update(&other);
    }

    
    
    
    
    void unionWith(const Range *other);
    static Range * intersect(const Range *lhs, const Range *rhs, bool *emptyRange);
    static Range * addTruncate(const Range *lhs, const Range *rhs);
    static Range * subTruncate(const Range *lhs, const Range *rhs);
    static Range * add(const Range *lhs, const Range *rhs);
    static Range * sub(const Range *lhs, const Range *rhs);
    static Range * mul(const Range *lhs, const Range *rhs);
    static Range * and_(const Range *lhs, const Range *rhs);
    static Range * shl(const Range *lhs, int32_t c);
    static Range * shr(const Range *lhs, int32_t c);

    static bool precisionLossMul(const Range *lhs, const Range *rhs);
    static bool negativeZeroMul(const Range *lhs, const Range *rhs);

    inline void makeLowerInfinite() {
        lower_infinite_ = true;
        lower_ = JSVAL_INT_MIN;
    }
    inline void makeUpperInfinite() {
        upper_infinite_ = true;
        upper_ = JSVAL_INT_MAX;
    }
    inline void makeRangeInfinite() {
        makeLowerInfinite();
        makeUpperInfinite();
    }

    inline bool isLowerInfinite() const {
        return lower_infinite_;
    }
    inline bool isUpperInfinite() const {
        return upper_infinite_;
    }

    inline bool isFinite() const {
        return !isLowerInfinite() && !isUpperInfinite();
    }

    inline int32_t lower() const {
        return lower_;
    }

    inline int32_t upper() const {
        return upper_;
    }

    inline void setLower(int64_t x) {
        if (x > JSVAL_INT_MAX) { 
            lower_ = JSVAL_INT_MAX;
        } else if (x < JSVAL_INT_MIN) {
            makeLowerInfinite();
        } else {
            lower_ = (int32_t)x;
            lower_infinite_ = false;
        }
    }
    inline void setUpper(int64_t x) {
        if (x > JSVAL_INT_MAX) {
            makeUpperInfinite();
        } else if (x < JSVAL_INT_MIN) { 
            upper_ = JSVAL_INT_MIN;
        } else {
            upper_ = (int32_t)x;
            upper_infinite_ = false;
        }
    }
    void set(int64_t l, int64_t h) {
        setLower(l);
        setUpper(h);
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

