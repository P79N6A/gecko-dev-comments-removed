





#ifndef jit_IonAnalysis_h
#define jit_IonAnalysis_h

#ifdef JS_ION



#include "jit/IonAllocPolicy.h"
#include "jit/MIR.h"

namespace js {
namespace jit {

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
MakeMRegExpHoistable(MIRGraph &graph);

bool
RenumberBlocks(MIRGraph &graph);

bool
BuildDominatorTree(MIRGraph &graph);

bool
BuildPhiReverseMapping(MIRGraph &graph);

void
AssertBasicGraphCoherency(MIRGraph &graph);

void
AssertGraphCoherency(MIRGraph &graph);

void
AssertExtendedGraphCoherency(MIRGraph &graph);

bool
EliminateRedundantChecks(MIRGraph &graph);

bool
UnsplitEdges(LIRGraph *lir);

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
    LinearSum(TempAllocator &alloc)
      : terms_(alloc),
        constant_(0)
    {
    }

    LinearSum(const LinearSum &other)
      : terms_(other.terms_.allocPolicy()),
        constant_(other.constant_)
    {
        terms_.appendAll(other.terms_);
    }

    bool multiply(int32_t scale);
    bool add(const LinearSum &other);
    bool add(MDefinition *term, int32_t scale);
    bool add(int32_t constant);

    int32_t constant() const { return constant_; }
    size_t numTerms() const { return terms_.length(); }
    LinearTerm term(size_t i) const { return terms_[i]; }

    void print(Sprinter &sp) const;
    void dump(FILE *) const;
    void dump() const;

  private:
    Vector<LinearTerm, 2, IonAllocPolicy> terms_;
    int32_t constant_;
};

bool
AnalyzeNewScriptProperties(JSContext *cx, JSFunction *fun,
                           types::TypeObject *type, HandleObject baseobj,
                           Vector<types::TypeNewScript::Initializer> *initializerList);

bool
AnalyzeArgumentsUsage(JSContext *cx, JSScript *script);

} 
} 

#endif 

#endif 
