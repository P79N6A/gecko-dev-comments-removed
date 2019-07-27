





#include "compiler/translator/IntermNode.h"

class TAliveTraverser : public TIntermTraverser {
public:
    TAliveTraverser(TQualifier q) : TIntermTraverser(true, false, false, true), found(false), qualifier(q)
    {
    }

	bool wasFound() { return found; }

protected:
    bool found;
    TQualifier qualifier;

    void visitSymbol(TIntermSymbol*);
    bool visitSelection(Visit, TIntermSelection*);
};









bool QualifierWritten(TIntermNode* node, TQualifier qualifier)
{
    TAliveTraverser it(qualifier);

    if (node)
        node->traverse(&it);

    return it.wasFound();
}

void TAliveTraverser::visitSymbol(TIntermSymbol* node)
{
    
    
    
    if (node->getQualifier() == qualifier)
        found = true;
}

bool TAliveTraverser::visitSelection(Visit preVisit, TIntermSelection* node)
{
    if (wasFound())
        return false;

    return true;
}
