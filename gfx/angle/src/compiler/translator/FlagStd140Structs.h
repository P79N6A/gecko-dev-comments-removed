





#ifndef COMPILER_FLAGSTD140STRUCTS_H_
#define COMPILER_FLAGSTD140STRUCTS_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{




class FlagStd140Structs : public TIntermTraverser
{
  public:
    const std::vector<TIntermTyped *> getFlaggedNodes() const { return mFlaggedNodes; }

  protected:
    virtual bool visitBinary(Visit visit, TIntermBinary *binaryNode);
    virtual void visitSymbol(TIntermSymbol *symbol);

  private:
    bool isInStd140InterfaceBlock(TIntermTyped *node) const;

    std::vector<TIntermTyped *> mFlaggedNodes;
};

std::vector<TIntermTyped *> FlagStd140ValueStructs(TIntermNode *node);

}

#endif 
