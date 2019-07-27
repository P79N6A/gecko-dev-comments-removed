







#ifndef COMPILER_SEARCHSYMBOL_H_
#define COMPILER_SEARCHSYMBOL_H_

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/ParseContext.h"

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
