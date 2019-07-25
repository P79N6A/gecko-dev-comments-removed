










































#ifndef mozilla_css_RestyleTracker_h
#define mozilla_css_RestyleTracker_h

#include "mozilla/dom/Element.h"
#include "nsDataHashtable.h"
#include "nsIFrame.h"

class nsCSSFrameConstructor;

namespace mozilla {
namespace css {

class RestyleTracker {
public:
  typedef mozilla::dom::Element Element;

  RestyleTracker(PRUint32 aRestyleBits,
                 nsCSSFrameConstructor* aFrameConstructor) :
    mRestyleBits(aRestyleBits), mFrameConstructor(aFrameConstructor),
    mHaveLaterSiblingRestyles(PR_FALSE)
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

  bool Init() {
    return mPendingRestyles.Init();
  }

  PRUint32 Count() const {
    return mPendingRestyles.Count();
  }

  



  bool AddPendingRestyle(Element* aElement, nsRestyleHint aRestyleHint,
                           nsChangeHint aMinChangeHint);

  


  void ProcessRestyles();

  
  PRUint32 RestyleBit() const {
    return mRestyleBits & ELEMENT_PENDING_RESTYLE_FLAGS;
  }

  
  PRUint32 RootBit() const {
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

  typedef nsDataHashtable<nsISupportsHashKey, RestyleData> PendingRestyleTable;
  typedef nsAutoTArray< nsRefPtr<Element>, 32> RestyleRootArray;
  
  
  
  PRUint32 mRestyleBits;
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
