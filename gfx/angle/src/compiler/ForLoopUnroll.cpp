





#include "compiler/ForLoopUnroll.h"

void ForLoopUnroll::FillLoopIndexInfo(TIntermLoop* node, TLoopIndexInfo& info)
{
    ASSERT(node->getType() == ELoopFor);
    ASSERT(node->getUnrollFlag());

    TIntermNode* init = node->getInit();
    ASSERT(init != NULL);
    TIntermAggregate* decl = init->getAsAggregate();
    ASSERT((decl != NULL) && (decl->getOp() == EOpDeclaration));
    TIntermSequence& declSeq = decl->getSequence();
    ASSERT(declSeq.size() == 1);
    TIntermBinary* declInit = declSeq[0]->getAsBinaryNode();
    ASSERT((declInit != NULL) && (declInit->getOp() == EOpInitialize));
    TIntermSymbol* symbol = declInit->getLeft()->getAsSymbolNode();
    ASSERT(symbol != NULL);
    ASSERT(symbol->getBasicType() == EbtInt);

    info.id = symbol->getId();

    ASSERT(declInit->getRight() != NULL);
    TIntermConstantUnion* initNode = declInit->getRight()->getAsConstantUnion();
    ASSERT(initNode != NULL);

    info.initValue = evaluateIntConstant(initNode);
    info.currentValue = info.initValue;

    TIntermNode* cond = node->getCondition();
    ASSERT(cond != NULL);
    TIntermBinary* binOp = cond->getAsBinaryNode();
    ASSERT(binOp != NULL);
    ASSERT(binOp->getRight() != NULL);
    ASSERT(binOp->getRight()->getAsConstantUnion() != NULL);

    info.incrementValue = getLoopIncrement(node);
    info.stopValue = evaluateIntConstant(
        binOp->getRight()->getAsConstantUnion());
    info.op = binOp->getOp();
}

void ForLoopUnroll::Step()
{
    ASSERT(mLoopIndexStack.size() > 0);
    TLoopIndexInfo& info = mLoopIndexStack[mLoopIndexStack.size() - 1];
    info.currentValue += info.incrementValue;
}

bool ForLoopUnroll::SatisfiesLoopCondition()
{
    ASSERT(mLoopIndexStack.size() > 0);
    TLoopIndexInfo& info = mLoopIndexStack[mLoopIndexStack.size() - 1];
    
    switch (info.op) {
      case EOpEqual:
        return (info.currentValue == info.stopValue);
      case EOpNotEqual:
        return (info.currentValue != info.stopValue);
      case EOpLessThan:
        return (info.currentValue < info.stopValue);
      case EOpGreaterThan:
        return (info.currentValue > info.stopValue);
      case EOpLessThanEqual:
        return (info.currentValue <= info.stopValue);
      case EOpGreaterThanEqual:
        return (info.currentValue >= info.stopValue);
      default:
        UNREACHABLE();
    }
    return false;
}

bool ForLoopUnroll::NeedsToReplaceSymbolWithValue(TIntermSymbol* symbol)
{
    for (TVector<TLoopIndexInfo>::iterator i = mLoopIndexStack.begin();
         i != mLoopIndexStack.end();
         ++i) {
        if (i->id == symbol->getId())
            return true;
    }
    return false;
}

int ForLoopUnroll::GetLoopIndexValue(TIntermSymbol* symbol)
{
    for (TVector<TLoopIndexInfo>::iterator i = mLoopIndexStack.begin();
         i != mLoopIndexStack.end();
         ++i) {
        if (i->id == symbol->getId())
            return i->currentValue;
    }
    UNREACHABLE();
    return false;
}

void ForLoopUnroll::Push(TLoopIndexInfo& info)
{
    mLoopIndexStack.push_back(info);
}

void ForLoopUnroll::Pop()
{
    mLoopIndexStack.pop_back();
}

int ForLoopUnroll::getLoopIncrement(TIntermLoop* node)
{
    TIntermNode* expr = node->getExpression();
    ASSERT(expr != NULL);
    
    
    
    
    
    
    
    
    
    TIntermUnary* unOp = expr->getAsUnaryNode();
    TIntermBinary* binOp = unOp ? NULL : expr->getAsBinaryNode();

    TOperator op = EOpNull;
    TIntermConstantUnion* incrementNode = NULL;
    if (unOp != NULL) {
        op = unOp->getOp();
    } else if (binOp != NULL) {
        op = binOp->getOp();
        ASSERT(binOp->getRight() != NULL);
        incrementNode = binOp->getRight()->getAsConstantUnion();
        ASSERT(incrementNode != NULL);
    }

    int increment = 0;
    
    switch (op) {
        case EOpPostIncrement:
        case EOpPreIncrement:
            ASSERT((unOp != NULL) && (binOp == NULL));
            increment = 1;
            break;
        case EOpPostDecrement:
        case EOpPreDecrement:
            ASSERT((unOp != NULL) && (binOp == NULL));
            increment = -1;
            break;
        case EOpAddAssign:
            ASSERT((unOp == NULL) && (binOp != NULL));
            increment = evaluateIntConstant(incrementNode);
            break;
        case EOpSubAssign:
            ASSERT((unOp == NULL) && (binOp != NULL));
            increment = - evaluateIntConstant(incrementNode);
            break;
        default:
            ASSERT(false);
    }

    return increment;
}

int ForLoopUnroll::evaluateIntConstant(TIntermConstantUnion* node)
{
    ASSERT((node != NULL) && (node->getUnionArrayPointer() != NULL));
    return node->getUnionArrayPointer()->getIConst();
}

