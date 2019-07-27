





#include "compiler/translator/ForLoopUnroll.h"

bool ForLoopUnrollMarker::visitBinary(Visit, TIntermBinary *node)
{
    if (mUnrollCondition != kSamplerArrayIndex)
        return true;

    
    
    
    switch (node->getOp())
    {
      case EOpIndexIndirect:
        if (node->getLeft() != NULL && node->getRight() != NULL && node->getLeft()->getAsSymbolNode())
        {
            TIntermSymbol *symbol = node->getLeft()->getAsSymbolNode();
            if (IsSampler(symbol->getBasicType()) && symbol->isArray() && !mLoopStack.empty())
            {
                mVisitSamplerArrayIndexNodeInsideLoop = true;
                node->getRight()->traverse(this);
                mVisitSamplerArrayIndexNodeInsideLoop = false;
                
                return false;
            }
        }
        break;
      default:
        break;
    }
    return true;
}

bool ForLoopUnrollMarker::visitLoop(Visit, TIntermLoop *node)
{
    if (mUnrollCondition == kIntegerIndex)
    {
        
        
        
        TIntermSequence *declSeq = node->getInit()->getAsAggregate()->getSequence();
        TIntermSymbol *symbol = (*declSeq)[0]->getAsBinaryNode()->getLeft()->getAsSymbolNode();
        if (symbol->getBasicType() == EbtInt)
            node->setUnrollFlag(true);
    }

    TIntermNode *body = node->getBody();
    if (body != NULL)
    {
        mLoopStack.push(node);
        body->traverse(this);
        mLoopStack.pop();
    }
    
    return false;
}

void ForLoopUnrollMarker::visitSymbol(TIntermSymbol* symbol)
{
    if (!mVisitSamplerArrayIndexNodeInsideLoop)
        return;
    TIntermLoop *loop = mLoopStack.findLoop(symbol);
    if (loop)
    {
        switch (symbol->getBasicType())
        {
          case EbtFloat:
            mSamplerArrayIndexIsFloatLoopIndex = true;
            break;
          case EbtInt:
            loop->setUnrollFlag(true);
            break;
          default:
            UNREACHABLE();
        }
    }
}
