



































#ifndef MOZILLA_DOMSVGPATHSEGLIST_H__
#define MOZILLA_DOMSVGPATHSEGLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsIDOMSVGPathSegList.h"
#include "nsSVGElement.h"
#include "nsTArray.h"
#include "SVGPathData.h" 

class nsIDOMSVGPathSeg;

namespace mozilla {

class DOMSVGPathSeg;
class SVGAnimatedPathSegList;


























class DOMSVGPathSegList : public nsIDOMSVGPathSegList,
                          public nsWrapperCache
{
  friend class DOMSVGPathSeg;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPathSegList)
  NS_DECL_NSIDOMSVGPATHSEGLIST

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(mElement);
  }

  
















  static already_AddRefed<DOMSVGPathSegList>
  GetDOMWrapper(void *aList,
                nsSVGElement *aElement,
                bool aIsAnimValList);

  




  static DOMSVGPathSegList*
  GetDOMWrapperIfExists(void *aList);

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() == InternalList().CountItems(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  















  void InternalListWillChangeTo(const SVGPathData& aNewValue);

  



  bool AttrIsAnimating() const;

private:

  



  DOMSVGPathSegList(nsSVGElement *aElement, bool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    SetIsDOMBinding();

    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPathSegList();

  nsSVGElement* Element() {
    return mElement.get();
  }

  
  bool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPathData& InternalList() const;

  SVGAnimatedPathSegList& InternalAList() const;

  
  
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

  bool mIsAnimValList;
};

} 

#endif 
