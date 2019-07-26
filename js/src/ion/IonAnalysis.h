






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

enum Observability {
    ConservativeObservability,
    AggressiveObservability
};

bool
EliminatePhis(MIRGenerator *mir, MIRGraph &graph, Observability observe);

bool
EliminateDeadResumePointOperands(MIRGenerator *mir, MIRGraph &graph);

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

void
AssertExtendedGraphCoherency(MIRGraph &graph);

bool
EliminateRedundantBoundsChecks(MIRGraph &graph);

class MDefinition;


struct SimpleLinearSum
{
    MDefinition *term;
    int32_t constant;

    SimpleLinearSum(MDefinition *term, int32_t constant)
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
    int32_t scale;

    LinearTerm(MDefinition *term, int32_t scale)
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

    bool multiply(int32_t scale);
    bool add(const LinearSum &other);
    bool add(MDefinition *term, int32_t scale);
    bool add(int32_t constant);

    int32_t constant() const { return constant_; }
    size_t numTerms() const { return terms_.length(); }
    LinearTerm term(size_t i) const { return terms_[i]; }

    void print(Sprinter &sp) const;

  private:
    Vector<LinearTerm, 2, IonAllocPolicy> terms_;
    int32_t constant_;
};

} 
} 

#endif 

