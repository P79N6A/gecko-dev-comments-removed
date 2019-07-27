





#include "compiler/translator/depgraph/DependencyGraph.h"



void TGraphNode::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->markVisited(this);
}

void TGraphParentNode::traverse(TDependencyGraphTraverser* graphTraverser)
{
    TGraphNode::traverse(graphTraverser);

    graphTraverser->incrementDepth();

    
    for (TGraphNodeSet::const_iterator iter = mDependentNodes.begin();
         iter != mDependentNodes.end();
         ++iter)
    {
        TGraphNode* node = *iter;
        if (!graphTraverser->isVisited(node))
            node->traverse(graphTraverser);
    }

    graphTraverser->decrementDepth();
}

void TGraphArgument::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitArgument(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphFunctionCall::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitFunctionCall(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphSymbol::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitSymbol(this);
    TGraphParentNode::traverse(graphTraverser);
}

void TGraphSelection::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitSelection(this);
    TGraphNode::traverse(graphTraverser);
}

void TGraphLoop::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitLoop(this);
    TGraphNode::traverse(graphTraverser);
}

void TGraphLogicalOp::traverse(TDependencyGraphTraverser* graphTraverser)
{
    graphTraverser->visitLogicalOp(this);
    TGraphNode::traverse(graphTraverser);
}
