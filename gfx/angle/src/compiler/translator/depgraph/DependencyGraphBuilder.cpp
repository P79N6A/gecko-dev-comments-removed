





#include "compiler/translator/depgraph/DependencyGraphBuilder.h"

void TDependencyGraphBuilder::build(TIntermNode *node, TDependencyGraph *graph)
{
    TDependencyGraphBuilder builder(graph);
    builder.build(node);
}

bool TDependencyGraphBuilder::visitAggregate(
    Visit visit, TIntermAggregate *intermAggregate)
{
    switch (intermAggregate->getOp())
    {
      case EOpFunction:
        visitFunctionDefinition(intermAggregate);
        break;
      case EOpFunctionCall:
        visitFunctionCall(intermAggregate);
        break;
      default:
        visitAggregateChildren(intermAggregate);
        break;
    }
    return false;
}

void TDependencyGraphBuilder::visitFunctionDefinition(
    TIntermAggregate *intermAggregate)
{
    
    if (intermAggregate->getName() != "main(")
        return;

    visitAggregateChildren(intermAggregate);
}



void TDependencyGraphBuilder::visitFunctionCall(
    TIntermAggregate *intermFunctionCall)
{
    TGraphFunctionCall *functionCall =
        mGraph->createFunctionCall(intermFunctionCall);

    
    int argumentNumber = 0;
    TIntermSequence *intermArguments = intermFunctionCall->getSequence();
    for (TIntermSequence::const_iterator iter = intermArguments->begin();
         iter != intermArguments->end();
         ++iter, ++argumentNumber)
    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        TIntermNode *intermArgument = *iter;
        intermArgument->traverse(this);

        if (TParentNodeSet *argumentNodes = mNodeSets.getTopSet())
        {
            TGraphArgument *argument = mGraph->createArgument(
                intermFunctionCall, argumentNumber);
            connectMultipleNodesToSingleNode(argumentNodes, argument);
            argument->addDependentNode(functionCall);
        }
    }

    
    
    
    
    
    
    mNodeSets.insertIntoTopSet(functionCall);
}

void TDependencyGraphBuilder::visitAggregateChildren(
    TIntermAggregate *intermAggregate)
{
    TIntermSequence *sequence = intermAggregate->getSequence();
    for (TIntermSequence::const_iterator iter = sequence->begin();
         iter != sequence->end(); ++iter)
    {
        TIntermNode *intermChild = *iter;
        intermChild->traverse(this);
    }
}

void TDependencyGraphBuilder::visitSymbol(TIntermSymbol *intermSymbol)
{
    
    
    TGraphSymbol *symbol = mGraph->getOrCreateSymbol(intermSymbol);
    mNodeSets.insertIntoTopSet(symbol);

    
    
    if (!mLeftmostSymbols.empty() && mLeftmostSymbols.top() != &mRightSubtree)
    {
        mLeftmostSymbols.pop();
        mLeftmostSymbols.push(symbol);
    }
}

bool TDependencyGraphBuilder::visitBinary(Visit visit, TIntermBinary *intermBinary)
{
    TOperator op = intermBinary->getOp();
    if (op == EOpInitialize || intermBinary->isAssignment())
        visitAssignment(intermBinary);
    else if (op == EOpLogicalAnd || op == EOpLogicalOr)
        visitLogicalOp(intermBinary);
    else
        visitBinaryChildren(intermBinary);

    return false;
}

void TDependencyGraphBuilder::visitAssignment(TIntermBinary *intermAssignment)
{
    TIntermTyped *intermLeft = intermAssignment->getLeft();
    if (!intermLeft)
        return;

    TGraphSymbol *leftmostSymbol = NULL;

    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        {
            TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mLeftSubtree);
            intermLeft->traverse(this);
            leftmostSymbol = mLeftmostSymbols.top();

            
            
            
            ASSERT(leftmostSymbol != &mLeftSubtree);
            ASSERT(leftmostSymbol != &mRightSubtree);
        }

        if (TIntermTyped *intermRight = intermAssignment->getRight())
        {
            TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
            intermRight->traverse(this);
        }

        if (TParentNodeSet *assignmentNodes = mNodeSets.getTopSet())
            connectMultipleNodesToSingleNode(assignmentNodes, leftmostSymbol);
    }

    
    
    
    
    
    
    
    mNodeSets.insertIntoTopSet(leftmostSymbol);
}

void TDependencyGraphBuilder::visitLogicalOp(TIntermBinary *intermLogicalOp)
{
    if (TIntermTyped *intermLeft = intermLogicalOp->getLeft())
    {
        TNodeSetPropagatingMaintainer nodeSetMaintainer(this);

        intermLeft->traverse(this);
        if (TParentNodeSet *leftNodes = mNodeSets.getTopSet())
        {
            TGraphLogicalOp *logicalOp = mGraph->createLogicalOp(intermLogicalOp);
            connectMultipleNodesToSingleNode(leftNodes, logicalOp);
        }
    }

    if (TIntermTyped *intermRight = intermLogicalOp->getRight())
    {
        TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
        intermRight->traverse(this);
    }
}

void TDependencyGraphBuilder::visitBinaryChildren(TIntermBinary *intermBinary)
{
    if (TIntermTyped *intermLeft = intermBinary->getLeft())
        intermLeft->traverse(this);

    if (TIntermTyped *intermRight = intermBinary->getRight())
    {
        TLeftmostSymbolMaintainer leftmostSymbolMaintainer(this, mRightSubtree);
        intermRight->traverse(this);
    }
}

bool TDependencyGraphBuilder::visitSelection(
    Visit visit, TIntermSelection *intermSelection)
{
    if (TIntermNode *intermCondition = intermSelection->getCondition())
    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        intermCondition->traverse(this);
        if (TParentNodeSet *conditionNodes = mNodeSets.getTopSet())
        {
            TGraphSelection *selection = mGraph->createSelection(intermSelection);
            connectMultipleNodesToSingleNode(conditionNodes, selection);
        }
    }

    if (TIntermNode *intermTrueBlock = intermSelection->getTrueBlock())
        intermTrueBlock->traverse(this);

    if (TIntermNode *intermFalseBlock = intermSelection->getFalseBlock())
        intermFalseBlock->traverse(this);

    return false;
}

bool TDependencyGraphBuilder::visitLoop(Visit visit, TIntermLoop *intermLoop)
{
    if (TIntermTyped *intermCondition = intermLoop->getCondition())
    {
        TNodeSetMaintainer nodeSetMaintainer(this);

        intermCondition->traverse(this);
        if (TParentNodeSet *conditionNodes = mNodeSets.getTopSet())
        {
            TGraphLoop *loop = mGraph->createLoop(intermLoop);
            connectMultipleNodesToSingleNode(conditionNodes, loop);
        }
    }

    if (TIntermNode* intermBody = intermLoop->getBody())
        intermBody->traverse(this);

    if (TIntermTyped *intermExpression = intermLoop->getExpression())
        intermExpression->traverse(this);

    return false;
}


void TDependencyGraphBuilder::connectMultipleNodesToSingleNode(
    TParentNodeSet *nodes, TGraphNode *node) const
{
    for (TParentNodeSet::const_iterator iter = nodes->begin();
         iter != nodes->end(); ++iter)
    {
        TGraphParentNode *currentNode = *iter;
        currentNode->addDependentNode(node);
    }
}
