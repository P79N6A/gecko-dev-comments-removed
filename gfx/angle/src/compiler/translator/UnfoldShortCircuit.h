







#ifndef COMPILER_UNFOLDSHORTCIRCUIT_H_
#define COMPILER_UNFOLDSHORTCIRCUIT_H_

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/ParseContext.h"

namespace sh
{
class OutputHLSL;

class UnfoldShortCircuit : public TIntermTraverser
{
  public:
    UnfoldShortCircuit(TParseContext &context, OutputHLSL *outputHLSL);

    void traverse(TIntermNode *node);
    bool visitBinary(Visit visit, TIntermBinary*);
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
