





#ifndef RELATION_H_
#define RELATION_H_

#include "AccIterator.h"

namespace mozilla {
namespace a11y {






struct RelationCopyHelper
{
  RelationCopyHelper(mozilla::a11y::AccIterable* aFirstIter,
                     mozilla::a11y::AccIterable* aLastIter) :
    mFirstIter(aFirstIter), mLastIter(aLastIter) { }

  mozilla::a11y::AccIterable* mFirstIter;
  mozilla::a11y::AccIterable* mLastIter;
};





class Relation
{
public:
  Relation() : mFirstIter(nullptr), mLastIter(nullptr) { }

  Relation(const RelationCopyHelper aRelation) :
    mFirstIter(aRelation.mFirstIter), mLastIter(aRelation.mLastIter) { }

  Relation(mozilla::a11y::AccIterable* aIter) :
    mFirstIter(aIter), mLastIter(aIter) { }

  Relation(Accessible* aAcc) :
    mFirstIter(nullptr), mLastIter(nullptr)
    { AppendTarget(aAcc); }

  Relation(DocAccessible* aDocument, nsIContent* aContent) :
    mFirstIter(nullptr), mLastIter(nullptr)
    { AppendTarget(aDocument, aContent); }

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

  inline void AppendIter(mozilla::a11y::AccIterable* aIter)
  {
    if (mLastIter)
      mLastIter->mNextIter = aIter;
    else
      mFirstIter = aIter;

    mLastIter = aIter;
  }

  


  inline void AppendTarget(Accessible* aAcc)
  {
    if (aAcc)
      AppendIter(new mozilla::a11y::SingleAccIterator(aAcc));
  }

  



  void AppendTarget(DocAccessible* aDocument, nsIContent* aContent)
  {
    if (aContent)
      AppendTarget(aDocument->GetAccessible(aContent));
  }

  


  inline Accessible* Next()
  {
    Accessible* target = nullptr;

    
    while (mFirstIter && !(target = mFirstIter->Next()))
      mFirstIter = mFirstIter->mNextIter;

    if (!mFirstIter)
      mLastIter = nullptr;

    return target;
  }

private:
  Relation& operator = (const Relation&);

  nsAutoPtr<mozilla::a11y::AccIterable> mFirstIter;
  mozilla::a11y::AccIterable* mLastIter;
};

} 
} 

#endif

