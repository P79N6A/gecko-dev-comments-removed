









#ifndef mozilla_RestyleTracker_h
#define mozilla_RestyleTracker_h

#include "mozilla/dom/Element.h"
#include "nsClassHashtable.h"
#include "nsContainerFrame.h"
#include "mozilla/SplayTree.h"
#include "mozilla/RestyleLogging.h"
#include "GeckoProfiler.h"

#if defined(MOZ_ENABLE_PROFILER_SPS) && !defined(MOZILLA_XPCOMRT_API)
#include "ProfilerBacktrace.h"
#endif

namespace mozilla {

class RestyleManager;
class ElementRestyler;






class OverflowChangedTracker
{
public:
  enum ChangeKind {
    




    TRANSFORM_CHANGED,
    



    CHILDREN_CHANGED,
  };

  OverflowChangedTracker() :
    mSubtreeRoot(nullptr)
  {}

  ~OverflowChangedTracker()
  {
    NS_ASSERTION(mEntryList.empty(), "Need to flush before destroying!");
  }

  










  void AddFrame(nsIFrame* aFrame, ChangeKind aChangeKind) {
    uint32_t depth = aFrame->GetDepthInFrameTree();
    Entry *entry = nullptr;
    if (!mEntryList.empty()) {
      entry = mEntryList.find(Entry(aFrame, depth));
    }
    if (entry == nullptr) {
      
      mEntryList.insert(new Entry(aFrame, depth, aChangeKind));
    } else {
      
      entry->mChangeKind = std::max(entry->mChangeKind, aChangeKind);
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

      bool overflowChanged = false;
      if (entry->mChangeKind == CHILDREN_CHANGED) {
        
        
        overflowChanged = frame->UpdateOverflow();
      } else {
        
        

#ifdef DEBUG
        bool hasInitialOverflowPropertyApplied = false;
        frame->Properties().Get(nsIFrame::DebugInitialOverflowPropertyApplied(),
                                 &hasInitialOverflowPropertyApplied);
        NS_ASSERTION(hasInitialOverflowPropertyApplied,
                     "InitialOverflowProperty must be set first.");
#endif

        nsOverflowAreas* overflow = 
          static_cast<nsOverflowAreas*>(frame->Properties().Get(nsIFrame::InitialOverflowProperty()));
        if (overflow) {
          
          
          nsOverflowAreas overflowCopy = *overflow;
          frame->FinishAndStoreOverflow(overflowCopy, frame->GetSize());
        } else {
          nsRect bounds(nsPoint(0, 0), frame->GetSize());
          nsOverflowAreas boundsOverflow;
          boundsOverflow.SetAllTo(bounds);
          frame->FinishAndStoreOverflow(boundsOverflow, bounds.Size());
        }

        
        overflowChanged = true;
      }

      
      
      
      if (overflowChanged) {
        nsIFrame *parent = frame->GetParent();
        if (parent && parent != mSubtreeRoot) {
          Entry* parentEntry = mEntryList.find(Entry(parent, entry->mDepth - 1));
          if (parentEntry) {
            parentEntry->mChangeKind = std::max(parentEntry->mChangeKind, CHILDREN_CHANGED);
          } else {
            mEntryList.insert(new Entry(parent, entry->mDepth - 1, CHILDREN_CHANGED));
          }
        }
      }
      delete entry;
    }
  }
  
private:
  struct Entry : SplayTreeNode<Entry>
  {
    Entry(nsIFrame* aFrame, uint32_t aDepth, ChangeKind aChangeKind = CHILDREN_CHANGED)
      : mFrame(aFrame)
      , mDepth(aDepth)
      , mChangeKind(aChangeKind)
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
    ChangeKind mChangeKind;
  };

  
  SplayTree<Entry, Entry> mEntryList;

  
  const nsIFrame* mSubtreeRoot;
};

class RestyleTracker {
public:
  typedef mozilla::dom::Element Element;

  friend class ElementRestyler; 

  explicit RestyleTracker(Element::FlagsType aRestyleBits)
    : mRestyleBits(aRestyleBits)
    , mHaveLaterSiblingRestyles(false)
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

  


  void DoProcessRestyles();

  
  uint32_t RestyleBit() const {
    return mRestyleBits & ELEMENT_PENDING_RESTYLE_FLAGS;
  }

  
  Element::FlagsType RootBit() const {
    return mRestyleBits & ~ELEMENT_PENDING_RESTYLE_FLAGS;
  }

  struct Hints {
    nsRestyleHint mRestyleHint;       
    nsChangeHint mChangeHint;         
  };

  struct RestyleData : Hints {
    RestyleData() {
      mRestyleHint = nsRestyleHint(0);
      mChangeHint = NS_STYLE_HINT_NONE;
    }

    RestyleData(nsRestyleHint aRestyleHint, nsChangeHint aChangeHint) {
      mRestyleHint = aRestyleHint;
      mChangeHint = aChangeHint;
    }

    
    
    
    
    nsTArray<nsRefPtr<Element>> mDescendants;
#if defined(MOZ_ENABLE_PROFILER_SPS) && !defined(MOZILLA_XPCOMRT_API)
    UniquePtr<ProfilerBacktrace> mBacktrace;
#endif
  };

  









  bool GetRestyleData(Element* aElement, nsAutoPtr<RestyleData>& aData);

  











  void AddRestyleRootsIfAwaitingRestyle(
                                  const nsTArray<nsRefPtr<Element>>& aElements);

  


  inline nsIDocument* Document() const;

#ifdef RESTYLE_LOGGING
  
  inline bool ShouldLogRestyle();
  inline int32_t& LoggingDepth();
#endif

private:
  bool AddPendingRestyleToTable(Element* aElement, nsRestyleHint aRestyleHint,
                                nsChangeHint aMinChangeHint);

  




  inline void ProcessOneRestyle(Element* aElement,
                                nsRestyleHint aRestyleHint,
                                nsChangeHint aChangeHint);

  typedef nsClassHashtable<nsISupportsHashKey, RestyleData> PendingRestyleTable;
  typedef nsAutoTArray< nsRefPtr<Element>, 32> RestyleRootArray;
  
  
  
  Element::FlagsType mRestyleBits;
  RestyleManager* mRestyleManager; 
  
  
  
  
  
  PendingRestyleTable mPendingRestyles;
  
  
  
  
  
  
  RestyleRootArray mRestyleRoots;
  
  
  
  bool mHaveLaterSiblingRestyles;
};

inline bool
RestyleTracker::AddPendingRestyleToTable(Element* aElement,
                                         nsRestyleHint aRestyleHint,
                                         nsChangeHint aMinChangeHint)
{
  RestyleData* existingData;

  
  
  
  if (aElement->HasFlag(RestyleBit())) {
    mPendingRestyles.Get(aElement, &existingData);
  } else {
    aElement->SetFlags(RestyleBit());
    existingData = nullptr;
  }

  if (!existingData) {
    RestyleData* rd = new RestyleData(aRestyleHint, aMinChangeHint);
#if defined(MOZ_ENABLE_PROFILER_SPS) && !defined(MOZILLA_XPCOMRT_API)
    if (profiler_feature_active("restyle")) {
      rd->mBacktrace.reset(profiler_get_backtrace());
    }
#endif
    mPendingRestyles.Put(aElement, rd);
    return false;
  }

  bool hadRestyleLaterSiblings =
    (existingData->mRestyleHint & eRestyle_LaterSiblings) != 0;
  existingData->mRestyleHint =
    nsRestyleHint(existingData->mRestyleHint | aRestyleHint);
  NS_UpdateHint(existingData->mChangeHint, aMinChangeHint);

  return hadRestyleLaterSiblings;
}

inline bool
RestyleTracker::AddPendingRestyle(Element* aElement,
                                  nsRestyleHint aRestyleHint,
                                  nsChangeHint aMinChangeHint)
{
  bool hadRestyleLaterSiblings =
    AddPendingRestyleToTable(aElement, aRestyleHint, aMinChangeHint);

  
  
  
  if ((aRestyleHint & ~eRestyle_LaterSiblings) ||
      (aMinChangeHint & nsChangeHint_ReconstructFrame)) {
    Element* cur = aElement;
    while (!cur->HasFlag(RootBit())) {
      nsIContent* parent = cur->GetFlattenedTreeParent();
      
      
      
      
      
      if (!parent || !parent->IsElement() ||
          
          
          
          
          
          
          (cur->IsInNativeAnonymousSubtree() && !parent->GetParent() &&
           cur->GetPrimaryFrame() &&
           cur->GetPrimaryFrame()->GetParent() != parent->GetPrimaryFrame())) {
        mRestyleRoots.AppendElement(aElement);
        cur = aElement;
        break;
      }
      cur = parent->AsElement();
    }
    
    
    
    aElement->SetFlags(RootBit());
    if (cur != aElement) {
      
      
      
      
      
      
      
      
      
      RestyleData* curData;
      mPendingRestyles.Get(cur, &curData);
      NS_ASSERTION(curData, "expected to find a RestyleData for cur");
      
      
      
      if (curData && !(curData->mRestyleHint & eRestyle_ForceDescendants)) {
        curData->mDescendants.AppendElement(aElement);
      }
    }
  }

  mHaveLaterSiblingRestyles =
    mHaveLaterSiblingRestyles || (aRestyleHint & eRestyle_LaterSiblings) != 0;
  return hadRestyleLaterSiblings;
}

} 

#endif 
