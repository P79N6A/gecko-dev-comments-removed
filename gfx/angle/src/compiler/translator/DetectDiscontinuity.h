









#ifndef COMPILER_DETECTDISCONTINUITY_H_
#define COMPILER_DETECTDISCONTINUITY_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{

class DetectLoopDiscontinuity : public TIntermTraverser
{
  public:
    bool traverse(TIntermNode *node);

  protected:
    bool visitBranch(Visit visit, TIntermBranch *node);
    bool visitLoop(Visit visit, TIntermLoop *loop);
    bool visitAggregate(Visit visit, TIntermAggregate *node);

    int mLoopDepth;
    bool mLoopDiscontinuity;
};

bool containsLoopDiscontinuity(TIntermNode *node);


class DetectGradientOperation : public TIntermTraverser
{
  public:
    bool traverse(TIntermNode *node);

  protected:
    bool visitUnary(Visit visit, TIntermUnary *node);
    bool visitAggregate(Visit visit, TIntermAggregate *node);

    bool mGradientOperation;
};

bool containsGradientOperation(TIntermNode *node);

}

#endif   
