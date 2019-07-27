





#ifndef mozilla_a11y_relation_h_
#define mozilla_a11y_relation_h_

#include "AccIterator.h"

#include "mozilla/Move.h"

namespace mozilla {
namespace a11y {





class Relation
{
public:
  Relation() : mFirstIter(nullptr), mLastIter(nullptr) { }

  explicit Relation(AccIterable* aIter) :
    mFirstIter(aIter), mLastIter(aIter) { }

  explicit Relation(Accessible* aAcc) :
    mFirstIter(nullptr), mLastIter(nullptr)
    { AppendTarget(aAcc); }

  Relation(DocAccessible* aDocument, nsIContent* aContent) :
    mFirstIter(nullptr), mLastIter(nullptr)
    { AppendTarget(aDocument, aContent); }

  Relation(Relation&& aOther) :
    mFirstIter(Move(aOther.mFirstIter)), mLastIter(aOther.mLastIter)
  {
    aOther.mLastIter = nullptr;
  }

  Relation& operator = (Relation&& aRH)
  {
    mFirstIter = Move(aRH.mFirstIter);
    mLastIter = aRH.mLastIter;
    aRH.mLastIter = nullptr;
    return *this;
  }

  inline void AppendIter(AccIterable* aIter)
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
      AppendIter(new SingleAccIterator(aAcc));
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
  Relation& operator = (const Relation&) MOZ_DELETE;
  Relation(const Relation&) MOZ_DELETE;

  nsAutoPtr<AccIterable> mFirstIter;
  AccIterable* mLastIter;
};

} 
} 

#endif

