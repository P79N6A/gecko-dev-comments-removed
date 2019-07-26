




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
#include "mozilla/Attributes.h"

class nsIDOMSVGPathSeg;

namespace mozilla {

class DOMSVGPathSeg;
class SVGAnimatedPathSegList;


























class DOMSVGPathSegList MOZ_FINAL : public nsIDOMSVGPathSegList,
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

  



  uint32_t Length() const {
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

  
  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex,
                                      uint32_t aInternalIndex,
                                      uint32_t aArgCountForItem);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex,
                                        uint32_t aArgCountForItem);

  
  
  
  void UpdateListIndicesFromIndex(uint32_t aStartingIndex,
                                  int32_t  aInternalDataIndexDelta);

  DOMSVGPathSeg*& ItemAt(uint32_t aIndex) {
    return mItems[aIndex].mItem;
  }

  








  struct ItemProxy {
    ItemProxy(){}
    ItemProxy(DOMSVGPathSeg *aItem, uint32_t aInternalDataIndex)
      : mItem(aItem)
      , mInternalDataIndex(aInternalDataIndex)
    {}

    DOMSVGPathSeg *mItem;
    uint32_t mInternalDataIndex;
  };

  
  
  nsTArray<ItemProxy> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  bool mIsAnimValList;
};

} 

#endif 
