




































#ifndef mozilla_css_RestyleTracker_h
#define mozilla_css_RestyleTracker_h






#include "mozilla/dom/Element.h"
#include "nsDataHashtable.h"

class nsCSSFrameConstructor;

namespace mozilla {
namespace css {

class RestyleTracker {
public:
  typedef mozilla::dom::Element Element;

  RestyleTracker(PRUint32 aRestyleBits,
                 nsCSSFrameConstructor* aFrameConstructor) :
    mRestyleBits(aRestyleBits), mFrameConstructor(aFrameConstructor)
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

  PRBool Init() {
    return mPendingRestyles.Init();
  }

  PRUint32 Count() const {
    return mPendingRestyles.Count();
  }

  


  inline void AddPendingRestyle(Element* aElement, nsRestyleHint aRestyleHint,
                                nsChangeHint aMinChangeHint);

  


  void ProcessRestyles();

  struct RestyleData {
    nsRestyleHint mRestyleHint;  
    nsChangeHint  mChangeHint;   
  };

  struct RestyleEnumerateData : public RestyleData {
    nsCOMPtr<Element> mElement;
  };

private:
  typedef nsDataHashtable<nsISupportsHashKey, RestyleData> PendingRestyleTable;
  
  
  
  PRUint32 mRestyleBits;
  nsCSSFrameConstructor* mFrameConstructor; 
  PendingRestyleTable mPendingRestyles;
};

inline void RestyleTracker::AddPendingRestyle(Element* aElement,
                                              nsRestyleHint aRestyleHint,
                                              nsChangeHint aMinChangeHint)
{
  RestyleData existingData;
  existingData.mRestyleHint = nsRestyleHint(0);
  existingData.mChangeHint = NS_STYLE_HINT_NONE;

  mPendingRestyles.Get(aElement, &existingData);

  existingData.mRestyleHint =
    nsRestyleHint(existingData.mRestyleHint | aRestyleHint);
  NS_UpdateHint(existingData.mChangeHint, aMinChangeHint);

  mPendingRestyles.Put(aElement, existingData);
}

} 
} 

#endif 
