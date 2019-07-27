







#include "compiler/translator/SearchSymbol.h"

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/OutputHLSL.h"

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
