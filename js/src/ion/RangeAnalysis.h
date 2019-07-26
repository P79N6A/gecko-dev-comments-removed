






#ifndef jsion_range_analysis_h__
#define jsion_range_analysis_h__

#include "wtf/Platform.h"
#include "MIR.h"
#include "CompileInfo.h"

namespace js {
namespace ion {

class MBasicBlock;
class MIRGraph;

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
};

class Range {
    private:
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        int32 lower_;
        bool lower_infinite_;
        int32 upper_;
        bool upper_infinite_;

    public:
        Range() :
            lower_(JSVAL_INT_MIN),
            lower_infinite_(true),
            upper_(JSVAL_INT_MAX),
            upper_infinite_(true)
        {}

        Range(int64_t l, int64_t h) {
            setLower(l);
            setUpper(h);
        }

        static int64_t abs64(int64_t x) {
#ifdef WTF_OS_WINDOWS
            return _abs64(x);
#else
            return llabs(x);
#endif
        }

        void printRange(FILE *fp);
        bool update(const Range *other);
        bool update(const Range &other) {
            return update(&other);
        }

        
        
        
        
        void unionWith(const Range *other);

        static Range intersect(const Range *lhs, const Range *rhs);
        static Range add(const Range *lhs, const Range *rhs);
        static Range sub(const Range *lhs, const Range *rhs);
        static Range mul(const Range *lhs, const Range *rhs);

        static Range shl(const Range *lhs, int32 c);
        static Range shr(const Range *lhs, int32 c);

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

        inline int32 lower() const {
            return lower_;
        }

        inline int32 upper() const {
            return upper_;
        }

        inline void setLower(int64_t x) {
            if (x > JSVAL_INT_MAX) { 
                lower_ = JSVAL_INT_MAX;
            } else if (x < JSVAL_INT_MIN) {
                makeLowerInfinite();
            } else {
                lower_ = (int32)x;
                lower_infinite_ = false;
            }
        }
        inline void setUpper(int64_t x) {
            if (x > JSVAL_INT_MAX) {
                makeUpperInfinite();
            } else if (x < JSVAL_INT_MIN) { 
                upper_ = JSVAL_INT_MIN;
            } else {
                upper_ = (int32)x;
                upper_infinite_ = false;
            }
        }
        void set(int64_t l, int64_t h) {
            setLower(l);
            setUpper(h);
        }
};

} 
} 

#endif 

