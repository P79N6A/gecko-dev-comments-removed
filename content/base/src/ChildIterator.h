





#ifndef ChildIterator_h
#define ChildIterator_h

#include "nsIContent.h"










#include "nsIContent.h"

namespace mozilla {
namespace dom {







class ExplicitChildIterator
{
public:
  ExplicitChildIterator(nsIContent* aParent, bool aStartAtBeginning = true)
    : mParent(aParent),
      mChild(nullptr),
      mDefaultChild(nullptr),
      mIndexInInserted(0),
      mIsFirst(aStartAtBeginning)
  {
  }

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
  FlattenedChildIterator(nsIContent* aParent)
    : ExplicitChildIterator(aParent), mXBLInvolved(false)
  {
    Init(false);
  }

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
