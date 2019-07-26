






#ifndef jsion_ion_analysis_h__
#define jsion_ion_analysis_h__



#include "IonAllocPolicy.h"
#include "MIR.h"

namespace js {
namespace ion {

class MIRGenerator;
class MIRGraph;

bool
SplitCriticalEdges(MIRGraph &graph);

bool
EliminatePhis(MIRGenerator *mir, MIRGraph &graph);

bool
EliminateDeadCode(MIRGenerator *mir, MIRGraph &graph);

bool
ApplyTypeInformation(MIRGenerator *mir, MIRGraph &graph);

bool
RenumberBlocks(MIRGraph &graph);

bool
BuildDominatorTree(MIRGraph &graph);

bool
BuildPhiReverseMapping(MIRGraph &graph);

void
AssertGraphCoherency(MIRGraph &graph);

bool
EliminateRedundantBoundsChecks(MIRGraph &graph);

class MDefinition;


struct SimpleLinearSum
{
    MDefinition *term;
    int32 constant;

    SimpleLinearSum(MDefinition *term, int32 constant)
        : term(term), constant(constant)
    {}
};

SimpleLinearSum
ExtractLinearSum(MDefinition *ins);

bool
ExtractLinearInequality(MTest *test, BranchDirection direction,
                        SimpleLinearSum *plhs, MDefinition **prhs, bool *plessEqual);

struct LinearTerm
{
    MDefinition *term;
    int32 scale;

    LinearTerm(MDefinition *term, int32 scale)
      : term(term), scale(scale)
    {
    }
};


class LinearSum
{
  public:
    LinearSum()
      : constant_(0)
    {
    }

    LinearSum(const LinearSum &other)
      : constant_(other.constant_)
    {
        for (size_t i = 0; i < other.terms_.length(); i++)
            terms_.append(other.terms_[i]);
    }

    bool multiply(int32 scale);
    bool add(const LinearSum &other);
    bool add(MDefinition *term, int32 scale);
    bool add(int32 constant);

    int32 constant() const { return constant_; }
    size_t numTerms() const { return terms_.length(); }
    LinearTerm term(size_t i) const { return terms_[i]; }

    void print(Sprinter &sp) const;

  private:
    Vector<LinearTerm, 2, IonAllocPolicy> terms_;
    int32 constant_;
};

} 
} 

#endif 

