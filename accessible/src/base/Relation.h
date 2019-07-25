






































#ifndef RELATION_H_
#define RELATION_H_

#include "AccIterator.h"






struct RelationCopyHelper
{
  RelationCopyHelper(AccIterable* aFirstIter, AccIterable* aLastIter) :
    mFirstIter(aFirstIter), mLastIter(aLastIter) { }

  AccIterable* mFirstIter;
  AccIterable* mLastIter;
};





class Relation
{
public:
  Relation() : mFirstIter(nsnull), mLastIter(nsnull) { }

  Relation(const RelationCopyHelper aRelation) :
    mFirstIter(aRelation.mFirstIter), mLastIter(aRelation.mLastIter) { }

  Relation(AccIterable* aIter) : mFirstIter(aIter), mLastIter(aIter) { }

  Relation(nsAccessible* aAcc) :
    mFirstIter(nsnull), mLastIter(nsnull)
    { AppendTarget(aAcc); }

  Relation(nsIContent* aContent) :
    mFirstIter(nsnull), mLastIter(nsnull)
    { AppendTarget(aContent); }

  Relation& operator = (const RelationCopyHelper& aRH)
  {
    mFirstIter = aRH.mFirstIter;
    mLastIter = aRH.mLastIter;
    return *this;
  }

  Relation& operator = (Relation& aRelation)
  {
    mFirstIter = aRelation.mFirstIter;
    mLastIter = aRelation.mLastIter;
    return *this;
  }

  operator RelationCopyHelper()
  {
    return RelationCopyHelper(mFirstIter.forget(), mLastIter);
  }

  inline void AppendIter(AccIterable* aIter)
  {
    if (mLastIter)
      mLastIter->mNextIter = aIter;
    else
      mFirstIter = aIter;

    mLastIter = aIter;
  }

  


  inline void AppendTarget(nsAccessible* aAcc)
  {
    if (aAcc)
      AppendIter(new SingleAccIterator(aAcc));
  }

  



  inline void AppendTarget(nsIContent* aContent)
  {
    if (aContent)
      AppendTarget(GetAccService()->GetAccessible(aContent, nsnull));
  }

  


  inline nsAccessible* Next()
  {
    nsAccessible* target = nsnull;

    
    while (mFirstIter && !(target = mFirstIter->Next()))
      mFirstIter = mFirstIter->mNextIter;

    if (!mFirstIter)
      mLastIter = nsnull;

    return target;
  }

private:
  Relation& operator = (const Relation&);

  nsAutoPtr<AccIterable> mFirstIter;
  AccIterable* mLastIter;
};

#endif

