







#include "compiler/SearchSymbol.h"

#include "compiler/InfoSink.h"
#include "compiler/OutputHLSL.h"

namespace sh
{
SearchSymbol::SearchSymbol(const TString &symbol) : mSymbol(symbol)
{
    match = false;
}

void SearchSymbol::traverse(TIntermNode *node)
{
    node->traverse(this);
}

void SearchSymbol::visitSymbol(TIntermSymbol *symbolNode)
{
    if (symbolNode->getSymbol() == mSymbol)
    {
        match = true;
    }
}

bool SearchSymbol::foundMatch() const
{
    return match;
}
}
