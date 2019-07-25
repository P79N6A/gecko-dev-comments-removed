



































#ifndef MOZILLA_DOMSVGPOINTLIST_H__
#define MOZILLA_DOMSVGPOINTLIST_H__

#include "nsIDOMSVGPointList.h"
#include "SVGPointList.h"
#include "SVGPoint.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsSVGElement;

namespace mozilla {

class DOMSVGPoint;
class SVGAnimatedPointList;


























class DOMSVGPointList : public nsIDOMSVGPointList
{
  friend class DOMSVGPoint;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGPointList)
  NS_DECL_NSIDOMSVGPOINTLIST

  
















  static already_AddRefed<DOMSVGPointList>
  GetDOMWrapper(void *aList,
                nsSVGElement *aElement,
                PRBool aIsAnimValList);

  




  static DOMSVGPointList*
  GetDOMWrapperIfExists(void *aList);

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() ==
                        const_cast<DOMSVGPointList*>(this)->InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  nsIDOMSVGPoint* GetItemWithoutAddRef(PRUint32 aIndex);

  















  void InternalListWillChangeTo(const SVGPointList& aNewValue);

  



  PRBool AttrIsAnimating() const;

private:

  



  DOMSVGPointList(nsSVGElement *aElement, PRBool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPointList();

  nsSVGElement* Element() {
    return mElement.get();
  }

  
  PRBool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPointList& InternalList();

  SVGAnimatedPointList& InternalAList();

  
  void EnsureItemAt(PRUint32 aIndex);

  void MaybeInsertNullInAnimValListAt(PRUint32 aIndex);
  void MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex);

  
  
  nsTArray<DOMSVGPoint*> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  PRPackedBool mIsAnimValList;
};

} 

#endif 
