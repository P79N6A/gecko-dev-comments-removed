








#ifndef COMPILER_REWRITE_ELSE_BLOCKS_H_
#define COMPILER_REWRITE_ELSE_BLOCKS_H_

#include "compiler/translator/intermediate.h"

namespace sh
{

class ElseBlockRewriter : public TIntermTraverser
{
  public:
      ElseBlockRewriter()
          : TIntermTraverser(false, false, true, false)
          , mTemporaryIndex(0)
      {}

  protected:
    bool visitAggregate(Visit visit, TIntermAggregate *aggregate);

  private:
    int mTemporaryIndex;

    TIntermNode *rewriteSelection(TIntermSelection *selection);
};

void RewriteElseBlocks(TIntermNode *node);

}

#endif 
