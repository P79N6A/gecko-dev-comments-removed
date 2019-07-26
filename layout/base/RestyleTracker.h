









#ifndef mozilla_css_RestyleTracker_h
#define mozilla_css_RestyleTracker_h

#include "mozilla/dom/Element.h"
#include "nsDataHashtable.h"
#include "nsIFrame.h"
#include "nsTPriorityQueue.h"
#include "mozilla/SplayTree.h"

class nsCSSFrameConstructor;

namespace mozilla {
namespace css {






class OverflowChangedTracker
{
public:

  ~OverflowChangedTracker()
  {
    NS_ASSERTION(mEntryList.empty(), "Need to flush before destroying!");
  }

  










  void AddFrame(nsIFrame* aFrame) {
    uint32_t depth = aFrame->GetDepthInFrameTree();
    if (mEntryList.empty() ||
        !mEntryList.contains(Entry(aFrame, depth, true))) {
      mEntryList.insert(new Entry(aFrame, depth, true));
    }
  }

  


  void RemoveFrame(nsIFrame* aFrame) {
    if (mEntryList.empty()) {
      return;
    }

    uint32_t depth = aFrame->GetDepthInFrameTree();
    if (mEntryList.contains(Entry(aFrame, depth, false))) {
      delete mEntryList.remove(Entry(aFrame, depth, false));
    }
  }

  





  void Flush() {
    while (!mEntryList.empty()) {
      Entry *entry = mEntryList.removeMin();

      nsIFrame *frame = entry->mFrame;

      bool updateParent = false;
      if (entry->mInitial) {
        nsOverflowAreas* pre = static_cast<nsOverflowAreas*>
          (frame->Properties().Get(frame->PreTransformOverflowAreasProperty()));
        if (pre) {
          
          
          nsOverflowAreas overflowAreas = *pre;
          frame->FinishAndStoreOverflow(overflowAreas, frame->GetSize());
          
          updateParent = true;
        }
      }

      
      
      if (!updateParent) {
        updateParent = frame->UpdateOverflow() || entry->mInitial;
      }
      if (updateParent) {
        nsIFrame *parent = frame->GetParent();
        if (parent) {
          if (!mEntryList.contains(Entry(parent, entry->mDepth - 1, false))) {
            mEntryList.insert(new Entry(parent, entry->mDepth - 1, false));
          }
        }
      }
      delete entry;
    }
  }
  
private:
  struct Entry : SplayTreeNode<Entry>
  {
    Entry(nsIFrame* aFrame, bool aInitial)
      : mFrame(aFrame)
      , mDepth(aFrame->GetDepthInFrameTree())
      , mInitial(aInitial)
    {}
    
    Entry(nsIFrame* aFrame, uint32_t aDepth, bool aInitial)
      : mFrame(aFrame)
      , mDepth(aDepth)
      , mInitial(aInitial)
    {}

    bool operator==(const Entry& aOther) const
    {
      return mFrame == aOther.mFrame;
    }
 
    



    bool operator<(const Entry& aOther) const
    {
      if (mDepth == aOther.mDepth) {
        return mFrame < aOther.mFrame;
      }
      return mDepth > aOther.mDepth; 
    }

    static int compare(const Entry& aOne, const Entry& aTwo)
    {
      if (aOne == aTwo) {
        return 0;
      } else if (aOne < aTwo) {
        return -1;
      } else {
        return 1;
      }
    }

    nsIFrame* mFrame;
    
    uint32_t mDepth;
    



    bool mInitial;
  };

  
  SplayTree<Entry, Entry> mEntryList;
};

class RestyleTracker {
public:
  typedef mozilla::dom::Element Element;

  RestyleTracker(uint32_t aRestyleBits,
                 nsCSSFrameConstructor* aFrameConstructor) :
    mRestyleBits(aRestyleBits), mFrameConstructor(aFrameConstructor),
    mHaveLaterSiblingRestyles(false)
  {
    NS_PRECONDITION((mRestyleBits & ~ELEMENT_ALL_RESTYLE_FLAGS) == 0,
                    "Why do we have these bits set?");
    NS_PRECONDITION((mRestyleBits & ELEMENT_PENDING_RESTYLE_FLAGS) != 0,
                    "Must have a restyle flag");
    NS_PRECONDITION((mRestyleBits & ELEMENT_PENDING_RESTYLE_FLAGS) !=
                      ELEMENT_PENDING_RESTYLE_FLAGS,
                    "Shouldn't have both restyle flags set");
    NS_PRECONDITION((mRestyleBits & ~ELEMENT_PENDING_RESTYLE_FLAGS) != 0,
                    "Must have root flag");
    NS_PRECONDITION((mRestyleBits & ~ELEMENT_PENDING_RESTYLE_FLAGS) !=
                    (ELEMENT_ALL_RESTYLE_FLAGS & ~ELEMENT_PENDING_RESTYLE_FLAGS),
                    "Shouldn't have both root flags");
  }

  void Init() {
    mPendingRestyles.Init();
  }

  uint32_t Count() const {
    return mPendingRestyles.Count();
  }

  



  bool AddPendingRestyle(Element* aElement, nsRestyleHint aRestyleHint,
                           nsChangeHint aMinChangeHint);

  


  void ProcessRestyles() {
    
    
    if (mPendingRestyles.Count()) {
      DoProcessRestyles();
    }
  }

  
  uint32_t RestyleBit() const {
    return mRestyleBits & ELEMENT_PENDING_RESTYLE_FLAGS;
  }

  
  uint32_t RootBit() const {
    return mRestyleBits & ~ELEMENT_PENDING_RESTYLE_FLAGS;
  }
  
  struct RestyleData {
    nsRestyleHint mRestyleHint;  
    nsChangeHint  mChangeHint;   
  };

  










  bool GetRestyleData(Element* aElement, RestyleData* aData);

  


  inline nsIDocument* Document() const;

  struct RestyleEnumerateData : public RestyleData {
    nsRefPtr<Element> mElement;
  };

private:
  




  inline void ProcessOneRestyle(Element* aElement,
                                nsRestyleHint aRestyleHint,
                                nsChangeHint aChangeHint);

  


  void DoProcessRestyles();

  typedef nsDataHashtable<nsISupportsHashKey, RestyleData> PendingRestyleTable;
  typedef nsAutoTArray< nsRefPtr<Element>, 32> RestyleRootArray;
  
  
  
  uint32_t mRestyleBits;
  nsCSSFrameConstructor* mFrameConstructor; 
  
  
  
  
  
  PendingRestyleTable mPendingRestyles;
  
  
  
  
  
  
  RestyleRootArray mRestyleRoots;
  
  
  
  bool mHaveLaterSiblingRestyles;
};

inline bool RestyleTracker::AddPendingRestyle(Element* aElement,
                                                nsRestyleHint aRestyleHint,
                                                nsChangeHint aMinChangeHint)
{
  RestyleData existingData;
  existingData.mRestyleHint = nsRestyleHint(0);
  existingData.mChangeHint = NS_STYLE_HINT_NONE;

  
  
  
  if (aElement->HasFlag(RestyleBit())) {
    mPendingRestyles.Get(aElement, &existingData);
  } else {
    aElement->SetFlags(RestyleBit());
  }

  bool hadRestyleLaterSiblings =
    (existingData.mRestyleHint & eRestyle_LaterSiblings) != 0;
  existingData.mRestyleHint =
    nsRestyleHint(existingData.mRestyleHint | aRestyleHint);
  NS_UpdateHint(existingData.mChangeHint, aMinChangeHint);

  mPendingRestyles.Put(aElement, existingData);

  
  
  
  if ((aRestyleHint & (eRestyle_Self | eRestyle_Subtree)) ||
      (aMinChangeHint & nsChangeHint_ReconstructFrame)) {
    for (const Element* cur = aElement; !cur->HasFlag(RootBit()); ) {
      nsIContent* parent = cur->GetFlattenedTreeParent();
      
      
      
      
      
      if (!parent || !parent->IsElement() ||
          
          
          
          
          
          
          (cur->IsInNativeAnonymousSubtree() && !parent->GetParent() &&
           cur->GetPrimaryFrame() &&
           cur->GetPrimaryFrame()->GetParent() != parent->GetPrimaryFrame())) {
        mRestyleRoots.AppendElement(aElement);
        break;
      }
      cur = parent->AsElement();
    }
    
    
    
    aElement->SetFlags(RootBit());
  }

  mHaveLaterSiblingRestyles =
    mHaveLaterSiblingRestyles || (aRestyleHint & eRestyle_LaterSiblings) != 0;
  return hadRestyleLaterSiblings;
}

} 
} 

#endif 
