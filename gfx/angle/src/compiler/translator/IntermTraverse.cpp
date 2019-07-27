





#include "compiler/translator/IntermNode.h"


















void TIntermSymbol::traverse(TIntermTraverser *it)
{
    it->visitSymbol(this);
}

void TIntermConstantUnion::traverse(TIntermTraverser *it)
{
    it->visitConstantUnion(this);
}




void TIntermBinary::traverse(TIntermTraverser *it)
{
    bool visit = true;

    
    
    
    if (it->preVisit)
        visit = it->visitBinary(PreVisit, this);

    
    
    
    if (visit)
    {
        it->incrementDepth(this);

        if (it->rightToLeft)
        {
            if (mRight)
                mRight->traverse(it);

            if (it->inVisit)
                visit = it->visitBinary(InVisit, this);

            if (visit && mLeft)
                mLeft->traverse(it);
        }
        else
        {
            if (mLeft)
                mLeft->traverse(it);

            if (it->inVisit)
                visit = it->visitBinary(InVisit, this);

            if (visit && mRight)
                mRight->traverse(it);
        }

        it->decrementDepth();
    }

    
    
    
    
    if (visit && it->postVisit)
        it->visitBinary(PostVisit, this);
}




void TIntermUnary::traverse(TIntermTraverser *it)
{
    bool visit = true;

    if (it->preVisit)
        visit = it->visitUnary(PreVisit, this);

    if (visit) {
        it->incrementDepth(this);
        mOperand->traverse(it);
        it->decrementDepth();
    }

    if (visit && it->postVisit)
        it->visitUnary(PostVisit, this);
}




void TIntermAggregate::traverse(TIntermTraverser *it)
{
    bool visit = true;

    if (it->preVisit)
        visit = it->visitAggregate(PreVisit, this);

    if (visit)
    {
        it->incrementDepth(this);

        if (it->rightToLeft)
        {
            for (TIntermSequence::reverse_iterator sit = mSequence.rbegin();
                 sit != mSequence.rend(); sit++)
            {
                (*sit)->traverse(it);

                if (visit && it->inVisit)
                {
                    if (*sit != mSequence.front())
                        visit = it->visitAggregate(InVisit, this);
                }
            }
        }
        else
        {
            for (TIntermSequence::iterator sit = mSequence.begin();
                 sit != mSequence.end(); sit++)
            {
                (*sit)->traverse(it);

                if (visit && it->inVisit)
                {
                    if (*sit != mSequence.back())
                        visit = it->visitAggregate(InVisit, this);
                }
            }
        }

        it->decrementDepth();
    }

    if (visit && it->postVisit)
        it->visitAggregate(PostVisit, this);
}




void TIntermSelection::traverse(TIntermTraverser *it)
{
    bool visit = true;

    if (it->preVisit)
        visit = it->visitSelection(PreVisit, this);

    if (visit)
    {
        it->incrementDepth(this);
        if (it->rightToLeft)
        {
            if (mFalseBlock)
                mFalseBlock->traverse(it);
            if (mTrueBlock)
                mTrueBlock->traverse(it);
            mCondition->traverse(it);
        }
        else
        {
            mCondition->traverse(it);
            if (mTrueBlock)
                mTrueBlock->traverse(it);
            if (mFalseBlock)
                mFalseBlock->traverse(it);
        }
        it->decrementDepth();
    }

    if (visit && it->postVisit)
        it->visitSelection(PostVisit, this);
}




void TIntermLoop::traverse(TIntermTraverser *it)
{
    bool visit = true;

    if (it->preVisit)
        visit = it->visitLoop(PreVisit, this);

    if (visit)
    {
        it->incrementDepth(this);

        if (it->rightToLeft)
        {
            if (mExpr)
                mExpr->traverse(it);

            if (mBody)
                mBody->traverse(it);

            if (mCond)
                mCond->traverse(it);

            if (mInit)
                mInit->traverse(it);
        }
        else
        {
            if (mInit)
                mInit->traverse(it);

            if (mCond)
                mCond->traverse(it);

            if (mBody)
                mBody->traverse(it);

            if (mExpr)
                mExpr->traverse(it);
        }

        it->decrementDepth();
    }

    if (visit && it->postVisit)
        it->visitLoop(PostVisit, this);
}




void TIntermBranch::traverse(TIntermTraverser *it)
{
    bool visit = true;

    if (it->preVisit)
        visit = it->visitBranch(PreVisit, this);

    if (visit && mExpression) {
        it->incrementDepth(this);
        mExpression->traverse(it);
        it->decrementDepth();
    }

    if (visit && it->postVisit)
        it->visitBranch(PostVisit, this);
}

void TIntermRaw::traverse(TIntermTraverser *it)
{
    it->visitRaw(this);
}
