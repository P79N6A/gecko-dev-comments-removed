





#include "compiler/MapLongVariableNames.h"

namespace {

TString mapLongName(int id, const TString& name)
{
    ASSERT(name.size() > MAX_IDENTIFIER_NAME_SIZE);
    TStringStream stream;
    stream << "webgl_" << id << "_";
    stream << name.substr(0, MAX_IDENTIFIER_NAME_SIZE - stream.str().size());
    return stream.str();
}

}  

void MapLongVariableNames::visitSymbol(TIntermSymbol* symbol)
{
    ASSERT(symbol != NULL);
    if (symbol->getSymbol().size() > MAX_IDENTIFIER_NAME_SIZE)
        symbol->setSymbol(mapLongName(symbol->getId(), symbol->getSymbol()));
}

void MapLongVariableNames::visitConstantUnion(TIntermConstantUnion*)
{
}

bool MapLongVariableNames::visitBinary(Visit, TIntermBinary*)
{
    return true;
}

bool MapLongVariableNames::visitUnary(Visit, TIntermUnary*)
{
    return true;
}

bool MapLongVariableNames::visitSelection(Visit, TIntermSelection*)
{
    return true;
}

bool MapLongVariableNames::visitAggregate(Visit, TIntermAggregate* node)
{
    return true;
}

bool MapLongVariableNames::visitLoop(Visit, TIntermLoop*)
{
    return true;
}

bool MapLongVariableNames::visitBranch(Visit, TIntermBranch*)
{
    return true;
}
