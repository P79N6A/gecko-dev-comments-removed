









#ifndef mozilla_RestyleTracker_h
#define mozilla_RestyleTracker_h

#include "mozilla/dom/Element.h"
#include "nsDataHashtable.h"
#include "nsIFrame.h"
#include "mozilla/SplayTree.h"

namespace mozilla {

class RestyleManager;






class OverflowChangedTracker
{
public:

  OverflowChangedTracker() :
    mSubtreeRoot(nullptr)
  {}

  ~OverflowChangedTracker()
  {
    NS_ASSERTION(mEntryList.empty(), "Need to flush before destroying!");
  }

  










  void AddFrame(nsIFrame* aFrame) {
    uint32_t depth = aFrame->GetDepthInFrameTree();
    if (mEntryList.empty() ||
        !mEntryList.find(Entry(aFrame, depth))) {
      
      
      
      mEntryList.insert(new Entry(aFrame, depth, STYLE_CHANGED));
    }
  }

  


  void RemoveFrame(nsIFrame* aFrame) {
    if (mEntryList.empty()) {
      return;
    }

    uint32_t depth = aFrame->GetDepthInFrameTree();
    if (mEntryList.find(Entry(aFrame, depth))) {
      delete mEntryList.remove(Entry(aFrame, depth));
    }
  }

  




  void SetSubtreeRoot(const nsIFrame* aSubtreeRoot) {
    mSubtreeRoot = aSubtreeRoot;
  }

  





  void Flush() {
    while (!mEntryList.empty()) {
      Entry *entry = mEntryList.removeMin();
      nsIFrame *frame = entry->mFrame;

      bool overflowChanged;
      if (entry->mFlags & CHILDREN_CHANGED) {
        
        overflowChanged = frame->UpdateOverflow();
      } else {
        nsOverflowAreas* pre = static_cast<nsOverflowAreas*>
          (frame->Properties().Get(frame->PreTransformOverflowAreasProperty()));
        if (pre) {
          
          
          
          
          
          nsOverflowAreas overflowAreas = *pre;
          frame->FinishAndStoreOverflow(overflowAreas, frame->GetSize());
          
          overflowChanged = true;
        } else {
          
          overflowChanged = frame->UpdateOverflow();
        }
      }

      
      
      
      if (overflowChanged || (entry->mFlags & STYLE_CHANGED)) {
        nsIFrame *parent = frame->GetParent();
        if (parent && parent != mSubtreeRoot) {
          Entry* parentEntry = mEntryList.find(Entry(parent, entry->mDepth - 1));
          if (parentEntry) {
            parentEntry->mFlags |= CHILDREN_CHANGED;
          } else {
            mEntryList.insert(new Entry(parent, entry->mDepth - 1, CHILDREN_CHANGED));
          }
        }
      }
      delete entry;
    }
  }
  
private:
  enum {
    



    CHILDREN_CHANGED = 0x01,
    





    STYLE_CHANGED = 0x02
  };
  struct Entry : SplayTreeNode<Entry>
  {
    Entry(nsIFrame* aFrame, uint32_t aDepth, uint8_t aFlags = 0)
      : mFrame(aFrame)
      , mDepth(aDepth)
      , mFlags(aFlags)
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
    uint8_t mFlags;
  };

  
  SplayTree<Entry, Entry> mEntryList;

  
  const nsIFrame* mSubtreeRoot;
};

class RestyleTracker {
public:
  typedef mozilla::dom::Element Element;

  RestyleTracker(uint32_t aRestyleBits) :
    mRestyleBits(aRestyleBits),
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

  void Init(RestyleManager* aRestyleManager) {
    mRestyleManager = aRestyleManager;
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
  RestyleManager* mRestyleManager; 
  
  
  
  
  
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

#endif
