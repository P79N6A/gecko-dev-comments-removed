





#include "compiler/translator/UnfoldShortCircuitAST.h"

namespace
{


TIntermSelection *UnfoldOR(TIntermTyped *x, TIntermTyped *y)
{
    const TType boolType(EbtBool, EbpUndefined);
    ConstantUnion *u = new ConstantUnion;
    u->setBConst(true);
    TIntermConstantUnion *trueNode = new TIntermConstantUnion(
        u, TType(EbtBool, EbpUndefined, EvqConst, 1));
    return new TIntermSelection(x, trueNode, y, boolType);
}


TIntermSelection *UnfoldAND(TIntermTyped *x, TIntermTyped *y)
{
    const TType boolType(EbtBool, EbpUndefined);
    ConstantUnion *u = new ConstantUnion;
    u->setBConst(false);
    TIntermConstantUnion *falseNode = new TIntermConstantUnion(
        u, TType(EbtBool, EbpUndefined, EvqConst, 1));
    return new TIntermSelection(x, y, falseNode, boolType);
}

}  

bool UnfoldShortCircuitAST::visitBinary(Visit visit, TIntermBinary *node)
{
    TIntermSelection *replacement = NULL;

    switch (node->getOp())
    {
      case EOpLogicalOr:
        replacement = UnfoldOR(node->getLeft(), node->getRight());
        break;
      case EOpLogicalAnd:
        replacement = UnfoldAND(node->getLeft(), node->getRight());
        break;
      default:
        break;
    }
    if (replacement)
    {
        replacements.push_back(
            NodeUpdateEntry(getParentNode(), node, replacement));
    }
    return true;
}

void UnfoldShortCircuitAST::updateTree()
{
    for (size_t ii = 0; ii < replacements.size(); ++ii)
    {
        const NodeUpdateEntry& entry = replacements[ii];
        ASSERT(entry.parent);
        bool replaced = entry.parent->replaceChildNode(
            entry.original, entry.replacement);
        ASSERT(replaced);

        
        
        
	
        for (size_t jj = ii + 1; jj < replacements.size(); ++jj)
        {
            NodeUpdateEntry& entry2 = replacements[jj];
            if (entry2.parent == entry.original)
                entry2.parent = entry.replacement;
        }
    }
}

