



































#ifndef MOZILLA_DOMSVGPOINTLIST_H__
#define MOZILLA_DOMSVGPOINTLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsIDOMSVGPointList.h"
#include "nsSVGElement.h"
#include "nsTArray.h"
#include "SVGPointList.h" 

class nsIDOMSVGPoint;

namespace mozilla {

class DOMSVGPoint;
class SVGAnimatedPointList;


























class DOMSVGPointList : public nsIDOMSVGPointList,
                        public nsWrapperCache
{
  friend class DOMSVGPoint;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPointList)
  NS_DECL_NSIDOMSVGPOINTLIST

  virtual JSObject* WrapObject(JSContext *cx, XPCWrappedNativeScope *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(mElement);
  }

  
















  static already_AddRefed<DOMSVGPointList>
  GetDOMWrapper(void *aList,
                nsSVGElement *aElement,
                bool aIsAnimValList);

  




  static DOMSVGPointList*
  GetDOMWrapperIfExists(void *aList);

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() == InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  















  void InternalListWillChangeTo(const SVGPointList& aNewValue);

  



  bool AttrIsAnimating() const;

private:

  



  DOMSVGPointList(nsSVGElement *aElement, bool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    SetIsProxy();

    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPointList();

  nsSVGElement* Element() {
    return mElement.get();
  }

  
  bool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPointList& InternalList() const;

  SVGAnimatedPointList& InternalAList() const;

  
  void EnsureItemAt(PRUint32 aIndex);

  void MaybeInsertNullInAnimValListAt(PRUint32 aIndex);
  void MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex);

  
  
  nsTArray<DOMSVGPoint*> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  bool mIsAnimValList;
};

} 

#endif 
