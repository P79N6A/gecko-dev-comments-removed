





#ifndef ChildIterator_h
#define ChildIterator_h

#include "nsIContent.h"










#include <stdint.h>
#include "nsAutoPtr.h"

class nsIContent;

namespace mozilla {
namespace dom {







class ExplicitChildIterator
{
public:
  explicit ExplicitChildIterator(nsIContent* aParent, bool aStartAtBeginning = true)
    : mParent(aParent),
      mChild(nullptr),
      mDefaultChild(nullptr),
      mIndexInInserted(0),
      mIsFirst(aStartAtBeginning)
  {
  }

  ExplicitChildIterator(const ExplicitChildIterator& aOther)
    : mParent(aOther.mParent), mChild(aOther.mChild),
      mDefaultChild(aOther.mDefaultChild),
      mShadowIterator(aOther.mShadowIterator ?
                      new ExplicitChildIterator(*aOther.mShadowIterator) :
                      nullptr),
      mIndexInInserted(aOther.mIndexInInserted), mIsFirst(aOther.mIsFirst) {}

  ExplicitChildIterator(ExplicitChildIterator&& aOther)
    : mParent(aOther.mParent), mChild(aOther.mChild),
      mDefaultChild(aOther.mDefaultChild),
      mShadowIterator(Move(aOther.mShadowIterator)),
      mIndexInInserted(aOther.mIndexInInserted), mIsFirst(aOther.mIsFirst) {}

  nsIContent* GetNextChild();

  
  
  
  
  bool Seek(nsIContent* aChildToFind, nsIContent* aBound = nullptr)
  {
    
    
    

    nsIContent* child;
    do {
      child = GetNextChild();
    } while (child && child != aChildToFind && child != aBound);

    return child == aChildToFind;
  }

  
  
  
  nsIContent* Get();

  
  
  nsIContent* GetPreviousChild();

protected:
  
  
  
  nsIContent* mParent;

  
  
  
  
  nsIContent* mChild;

  
  
  
  
  nsIContent* mDefaultChild;

  
  
  
  nsAutoPtr<ExplicitChildIterator> mShadowIterator;

  
  
  
  
  uint32_t mIndexInInserted;

  
  bool mIsFirst;
};




class FlattenedChildIterator : public ExplicitChildIterator
{
public:
  explicit FlattenedChildIterator(nsIContent* aParent)
    : ExplicitChildIterator(aParent), mXBLInvolved(false)
  {
    Init(false);
  }

  FlattenedChildIterator(FlattenedChildIterator&& aOther)
    : ExplicitChildIterator(Move(aOther)), mXBLInvolved(aOther.mXBLInvolved) {}

  FlattenedChildIterator(const FlattenedChildIterator& aOther)
    : ExplicitChildIterator(aOther), mXBLInvolved(aOther.mXBLInvolved) {}

  bool XBLInvolved() { return mXBLInvolved; }

protected:
  



  FlattenedChildIterator(nsIContent* aParent, bool aIgnoreXBL)
    : ExplicitChildIterator(aParent), mXBLInvolved(false)
  {
    Init(aIgnoreXBL);
  }

  void Init(bool aIgnoreXBL);

  
  
  bool mXBLInvolved;
};







class AllChildrenIterator : private FlattenedChildIterator
{
public:
  AllChildrenIterator(nsIContent* aNode, uint32_t aFlags) :
    FlattenedChildIterator(aNode, (aFlags & nsIContent::eAllButXBL)),
    mOriginalContent(aNode), mFlags(aFlags),
    mPhase(eNeedBeforeKid) {}

  AllChildrenIterator(AllChildrenIterator&& aOther)
    : FlattenedChildIterator(Move(aOther)),
      mOriginalContent(aOther.mOriginalContent),
      mAnonKids(Move(aOther.mAnonKids)), mFlags(aOther.mFlags),
      mPhase(aOther.mPhase)
#ifdef DEBUG
      , mMutationGuard(aOther.mMutationGuard)
#endif
      {}

#ifdef DEBUG
  ~AllChildrenIterator() { MOZ_ASSERT(!mMutationGuard.Mutated(0)); }
#endif

  nsIContent* GetNextChild();

private:
  enum IteratorPhase
  {
    eNeedBeforeKid,
    eNeedExplicitKids,
    eNeedAnonKids,
    eNeedAfterKid,
    eDone
  };

  nsIContent* mOriginalContent;
  nsTArray<nsIContent*> mAnonKids;
  uint32_t mFlags;
  IteratorPhase mPhase;
#ifdef DEBUG
  
  
  nsMutationGuard mMutationGuard;
#endif
};

} 
} 

#endif
