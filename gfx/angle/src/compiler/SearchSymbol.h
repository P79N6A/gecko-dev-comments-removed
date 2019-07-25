







#ifndef COMPILER_SEARCHSYMBOL_H_
#define COMPILER_SEARCHSYMBOL_H_

#include "compiler/intermediate.h"
#include "compiler/ParseHelper.h"

namespace sh
{
class SearchSymbol : public TIntermTraverser
{
  public:
    SearchSymbol(const TString &symbol);

    void traverse(TIntermNode *node);
    void visitSymbol(TIntermSymbol *symbolNode);

    bool foundMatch() const;

  protected:
    const TString &mSymbol;
    bool match;
};
}

#endif   
