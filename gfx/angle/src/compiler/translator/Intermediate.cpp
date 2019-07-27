









#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/translator/Intermediate.h"
#include "compiler/translator/RemoveTree.h"
#include "compiler/translator/SymbolTable.h"














TIntermSymbol *TIntermediate::addSymbol(
    int id, const TString &name, const TType &type, const TSourceLoc &line)
{
    TIntermSymbol *node = new TIntermSymbol(id, name, type);
    node->setLine(line);

    return node;
}






TIntermTyped *TIntermediate::addBinaryMath(
    TOperator op, TIntermTyped *left, TIntermTyped *right, const TSourceLoc &line)
{
    switch (op)
    {
      case EOpEqual:
      case EOpNotEqual:
        if (left->isArray())
            return NULL;
        break;
      case EOpLessThan:
      case EOpGreaterThan:
      case EOpLessThanEqual:
      case EOpGreaterThanEqual:
        if (left->isMatrix() || left->isArray() || left->isVector() ||
            left->getBasicType() == EbtStruct)
        {
            return NULL;
        }
        break;
      case EOpLogicalOr:
      case EOpLogicalXor:
      case EOpLogicalAnd:
        if (left->getBasicType() != EbtBool ||
            left->isMatrix() || left->isArray() || left->isVector())
        {
            return NULL;
        }
        break;
      case EOpAdd:
      case EOpSub:
      case EOpDiv:
      case EOpMul:
        if (left->getBasicType() == EbtStruct || left->getBasicType() == EbtBool)
            return NULL;
      default:
        break;
    }

    if (left->getBasicType() != right->getBasicType())
    {
        return NULL;
    }

    
    
    
    
    TIntermBinary *node = new TIntermBinary(op);
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (!node->promote(mInfoSink))
        return NULL;

    
    
    
    TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
    TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
    if (leftTempConstant && rightTempConstant)
    {
        TIntermTyped *typedReturnNode =
            leftTempConstant->fold(node->getOp(), rightTempConstant, mInfoSink);

        if (typedReturnNode)
            return typedReturnNode;
    }

    return node;
}






TIntermTyped *TIntermediate::addAssign(
    TOperator op, TIntermTyped *left, TIntermTyped *right, const TSourceLoc &line)
{
    if (left->getType().getStruct() || right->getType().getStruct())
    {
        if (left->getType() != right->getType())
        {
            return NULL;
        }
    }

    TIntermBinary *node = new TIntermBinary(op);
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (!node->promote(mInfoSink))
        return NULL;

    return node;
}








TIntermTyped *TIntermediate::addIndex(
    TOperator op, TIntermTyped *base, TIntermTyped *index, const TSourceLoc &line)
{
    TIntermBinary *node = new TIntermBinary(op);
    node->setLine(line);
    node->setLeft(base);
    node->setRight(index);

    

    return node;
}






TIntermTyped *TIntermediate::addUnaryMath(
    TOperator op, TIntermNode *childNode, const TSourceLoc &line)
{
    TIntermUnary *node;
    TIntermTyped *child = childNode->getAsTyped();

    if (child == NULL)
    {
        mInfoSink.info.message(EPrefixInternalError, line,
                               "Bad type in AddUnaryMath");
        return NULL;
    }

    switch (op)
    {
      case EOpLogicalNot:
        if (child->getType().getBasicType() != EbtBool ||
            child->getType().isMatrix() ||
            child->getType().isArray() ||
            child->getType().isVector())
        {
            return NULL;
        }
        break;

      case EOpPostIncrement:
      case EOpPreIncrement:
      case EOpPostDecrement:
      case EOpPreDecrement:
      case EOpNegative:
        if (child->getType().getBasicType() == EbtStruct ||
            child->getType().isArray())
        {
            return NULL;
        }
      default:
        break;
    }

    TIntermConstantUnion *childTempConstant = 0;
    if (child->getAsConstantUnion())
        childTempConstant = child->getAsConstantUnion();

    
    
    
    node = new TIntermUnary(op);
    node->setLine(line);
    node->setOperand(child);

    if (!node->promote(mInfoSink))
        return 0;

    if (childTempConstant)
    {
        TIntermTyped *newChild = childTempConstant->fold(op, 0, mInfoSink);

        if (newChild)
            return newChild;
    }

    return node;
}











TIntermAggregate *TIntermediate::setAggregateOperator(
    TIntermNode *node, TOperator op, const TSourceLoc &line)
{
    TIntermAggregate *aggNode;

    
    
    
    if (node)
    {
        aggNode = node->getAsAggregate();
        if (aggNode == NULL || aggNode->getOp() != EOpNull)
        {
            
            
            
            aggNode = new TIntermAggregate();
            aggNode->getSequence()->push_back(node);
        }
    }
    else
    {
        aggNode = new TIntermAggregate();
    }

    
    
    
    aggNode->setOp(op);
    aggNode->setLine(line);

    return aggNode;
}








TIntermAggregate *TIntermediate::growAggregate(
    TIntermNode *left, TIntermNode *right, const TSourceLoc &line)
{
    if (left == NULL && right == NULL)
        return NULL;

    TIntermAggregate *aggNode = NULL;
    if (left)
        aggNode = left->getAsAggregate();
    if (!aggNode || aggNode->getOp() != EOpNull)
    {
        aggNode = new TIntermAggregate;
        if (left)
            aggNode->getSequence()->push_back(left);
    }

    if (right)
        aggNode->getSequence()->push_back(right);

    aggNode->setLine(line);

    return aggNode;
}






TIntermAggregate *TIntermediate::makeAggregate(
    TIntermNode *node, const TSourceLoc &line)
{
    if (node == NULL)
        return NULL;

    TIntermAggregate *aggNode = new TIntermAggregate;
    aggNode->getSequence()->push_back(node);

    aggNode->setLine(line);

    return aggNode;
}








TIntermNode *TIntermediate::addSelection(
    TIntermTyped *cond, TIntermNodePair nodePair, const TSourceLoc &line)
{
    
    
    
    

    if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion())
    {
        if (cond->getAsConstantUnion()->getBConst(0) == true)
        {
            return nodePair.node1 ? setAggregateOperator(
                nodePair.node1, EOpSequence, nodePair.node1->getLine()) : NULL;
        }
        else
        {
            return nodePair.node2 ? setAggregateOperator(
                nodePair.node2, EOpSequence, nodePair.node2->getLine()) : NULL;
        }
    }

    TIntermSelection *node = new TIntermSelection(
        cond, nodePair.node1, nodePair.node2);
    node->setLine(line);

    return node;
}

TIntermTyped *TIntermediate::addComma(
    TIntermTyped *left, TIntermTyped *right, const TSourceLoc &line)
{
    if (left->getType().getQualifier() == EvqConst &&
        right->getType().getQualifier() == EvqConst)
    {
        return right;
    }
    else
    {
        TIntermTyped *commaAggregate = growAggregate(left, right, line);
        commaAggregate->getAsAggregate()->setOp(EOpComma);
        commaAggregate->setType(right->getType());
        commaAggregate->getTypePointer()->setQualifier(EvqTemporary);
        return commaAggregate;
    }
}








TIntermTyped *TIntermediate::addSelection(
    TIntermTyped *cond, TIntermTyped *trueBlock, TIntermTyped *falseBlock,
    const TSourceLoc &line)
{
    if (!cond || !trueBlock || !falseBlock ||
        trueBlock->getType() != falseBlock->getType())
    {
        return NULL;
    }

    
    
    

    if (cond->getAsConstantUnion() &&
        trueBlock->getAsConstantUnion() &&
        falseBlock->getAsConstantUnion())
    {
        if (cond->getAsConstantUnion()->getBConst(0))
            return trueBlock;
        else
            return falseBlock;
    }

    
    
    
    TIntermSelection *node = new TIntermSelection(
        cond, trueBlock, falseBlock, trueBlock->getType());
    node->getTypePointer()->setQualifier(EvqTemporary);
    node->setLine(line);

    return node;
}







TIntermConstantUnion *TIntermediate::addConstantUnion(
    ConstantUnion *unionArrayPointer, const TType &t, const TSourceLoc &line)
{
    TIntermConstantUnion *node = new TIntermConstantUnion(unionArrayPointer, t);
    node->setLine(line);

    return node;
}

TIntermTyped *TIntermediate::addSwizzle(
    TVectorFields &fields, const TSourceLoc &line)
{

    TIntermAggregate *node = new TIntermAggregate(EOpSequence);

    node->setLine(line);
    TIntermConstantUnion *constIntNode;
    TIntermSequence *sequenceVector = node->getSequence();
    ConstantUnion *unionArray;

    for (int i = 0; i < fields.num; i++)
    {
        unionArray = new ConstantUnion[1];
        unionArray->setIConst(fields.offsets[i]);
        constIntNode = addConstantUnion(
            unionArray, TType(EbtInt, EbpUndefined, EvqConst), line);
        sequenceVector->push_back(constIntNode);
    }

    return node;
}




TIntermNode *TIntermediate::addLoop(
    TLoopType type, TIntermNode *init, TIntermTyped *cond, TIntermTyped *expr,
    TIntermNode *body, const TSourceLoc &line)
{
    TIntermNode *node = new TIntermLoop(type, init, cond, expr, body);
    node->setLine(line);

    return node;
}




TIntermBranch* TIntermediate::addBranch(
    TOperator branchOp, const TSourceLoc &line)
{
    return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(
    TOperator branchOp, TIntermTyped *expression, const TSourceLoc &line)
{
    TIntermBranch *node = new TIntermBranch(branchOp, expression);
    node->setLine(line);

    return node;
}





bool TIntermediate::postProcess(TIntermNode *root)
{
    if (root == NULL)
        return true;

    
    
    
    TIntermAggregate *aggRoot = root->getAsAggregate();
    if (aggRoot && aggRoot->getOp() == EOpNull)
        aggRoot->setOp(EOpSequence);

    return true;
}




void TIntermediate::remove(TIntermNode *root)
{
    if (root)
        RemoveAllTreeNodes(root);
}
