



































#ifndef MOZILLA_DOMSVGPATHSEGLIST_H__
#define MOZILLA_DOMSVGPATHSEGLIST_H__

#include "nsIDOMSVGPathSegList.h"
#include "SVGPathData.h"
#include "SVGPathSegUtils.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsSVGElement;

namespace mozilla {

class DOMSVGPathSeg;
class SVGAnimatedPathSegList;


























class DOMSVGPathSegList : public nsIDOMSVGPathSegList
{
  friend class DOMSVGPathSeg;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGPathSegList)
  NS_DECL_NSIDOMSVGPATHSEGLIST

  
















  static already_AddRefed<DOMSVGPathSegList>
  GetDOMWrapper(void *aList,
                nsSVGElement *aElement,
                PRBool aIsAnimValList);

  




  static DOMSVGPathSegList*
  GetDOMWrapperIfExists(void *aList);

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() ==
                        const_cast<DOMSVGPathSegList*>(this)->InternalList().CountItems(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  nsIDOMSVGPathSeg* GetItemWithoutAddRef(PRUint32 aIndex);

  















  void InternalListWillChangeTo(const SVGPathData& aNewValue);

  



  PRBool AttrIsAnimating() const;

private:

  



  DOMSVGPathSegList(nsSVGElement *aElement, PRBool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPathSegList();

  nsSVGElement* Element() {
    return mElement.get();
  }

  
  PRBool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPathData& InternalList();

  SVGAnimatedPathSegList& InternalAList();

  
  
  void EnsureItemAt(PRUint32 aIndex);

  void MaybeInsertNullInAnimValListAt(PRUint32 aIndex,
                                      PRUint32 aInternalIndex,
                                      PRUint32 aArgCountForItem);
  void MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex,
                                        PRUint32 aArgCountForItem);

  
  
  
  void UpdateListIndicesFromIndex(PRUint32 aStartingIndex,
                                  PRInt32  aInternalDataIndexDelta);

  DOMSVGPathSeg*& ItemAt(PRUint32 aIndex) {
    return mItems[aIndex].mItem;
  }

  








  struct ItemProxy {
    ItemProxy(){}
    ItemProxy(DOMSVGPathSeg *aItem, PRUint32 aInternalDataIndex)
      : mItem(aItem)
      , mInternalDataIndex(aInternalDataIndex)
    {}

    DOMSVGPathSeg *mItem;
    PRUint32 mInternalDataIndex;
  };

  
  
  nsTArray<ItemProxy> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  PRPackedBool mIsAnimValList;
};

} 

#endif 
