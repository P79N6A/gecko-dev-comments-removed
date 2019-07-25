







#ifndef COMPILER_UNFOLDSELECT_H_
#define COMPILER_UNFOLDSELECT_H_

#include "compiler/intermediate.h"
#include "compiler/ParseHelper.h"

namespace sh
{
class OutputHLSL;

class UnfoldSelect : public TIntermTraverser
{
  public:
    UnfoldSelect(TParseContext &context, OutputHLSL *outputHLSL);

    void traverse(TIntermNode *node);
    bool visitSelection(Visit visit, TIntermSelection *node);
    bool visitLoop(Visit visit, TIntermLoop *node);

    int getNextTemporaryIndex();

  protected:
    TParseContext &mContext;
    OutputHLSL *const mOutputHLSL;

    int mTemporaryIndex;
};
}

#endif   
