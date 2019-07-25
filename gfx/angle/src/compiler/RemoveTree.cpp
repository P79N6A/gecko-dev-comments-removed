





#include "compiler/intermediate.h"
#include "compiler/RemoveTree.h"





class RemoveTree : public TIntermTraverser
{
public:
	RemoveTree() : TIntermTraverser(false, false, true)
	{
	}

protected:
	void visitSymbol(TIntermSymbol*);
	void visitConstantUnion(TIntermConstantUnion*);
	bool visitBinary(Visit visit, TIntermBinary*);
	bool visitUnary(Visit visit, TIntermUnary*);
	bool visitSelection(Visit visit, TIntermSelection*);
	bool visitAggregate(Visit visit, TIntermAggregate*);
};

void RemoveTree::visitSymbol(TIntermSymbol* node)
{
	delete node;
}

bool RemoveTree::visitBinary(Visit visit, TIntermBinary* node)
{
	delete node;

	return true;
}

bool RemoveTree::visitUnary(Visit visit, TIntermUnary* node)
{
    delete node;

	return true;
}

bool RemoveTree::visitAggregate(Visit visit, TIntermAggregate* node)
{
	delete node;

	return true;
}

bool RemoveTree::visitSelection(Visit visit, TIntermSelection* node)
{
	delete node;

	return true;
}

void RemoveTree::visitConstantUnion(TIntermConstantUnion* node)
{
	delete node;
}




void RemoveAllTreeNodes(TIntermNode* root)
{
    RemoveTree it;

    root->traverse(&it);
}

