





#ifndef COMPILER_FORLOOPUNROLL_H_
#define COMPILER_FORLOOPUNROLL_H_

#include "compiler/translator/LoopInfo.h"







class ForLoopUnrollMarker : public TIntermTraverser
{
  public:
    enum UnrollCondition
    {
        kIntegerIndex,
        kSamplerArrayIndex
    };

    ForLoopUnrollMarker(UnrollCondition condition)
        : mUnrollCondition(condition),
          mSamplerArrayIndexIsFloatLoopIndex(false),
          mVisitSamplerArrayIndexNodeInsideLoop(false)
    {
    }

    virtual bool visitBinary(Visit, TIntermBinary *node);
    virtual bool visitLoop(Visit, TIntermLoop *node);
    virtual void visitSymbol(TIntermSymbol *node);

    bool samplerArrayIndexIsFloatLoopIndex() const
    {
        return mSamplerArrayIndexIsFloatLoopIndex;
    }

  private:
    UnrollCondition mUnrollCondition;
    TLoopStack mLoopStack;
    bool mSamplerArrayIndexIsFloatLoopIndex;
    bool mVisitSamplerArrayIndexNodeInsideLoop;
};

#endif
