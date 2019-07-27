





#ifndef COMPILER_RENAME_FUNCTION
#define COMPILER_RENAME_FUNCTION

#include "compiler/translator/IntermNode.h"




class RenameFunction : public TIntermTraverser
{
public:
    RenameFunction(const TString& oldFunctionName, const TString& newFunctionName)
    : TIntermTraverser(true, false, false)
    , mOldFunctionName(oldFunctionName)
    , mNewFunctionName(newFunctionName) {}

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node)
    {
        TOperator op = node->getOp();
        if ((op == EOpFunction || op == EOpFunctionCall) && node->getName() == mOldFunctionName)
            node->setName(mNewFunctionName);
        return true;
    }

private:
    const TString mOldFunctionName;
    const TString mNewFunctionName;
};

#endif  
