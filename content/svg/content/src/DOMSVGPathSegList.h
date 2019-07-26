




#ifndef MOZILLA_DOMSVGPATHSEGLIST_H__
#define MOZILLA_DOMSVGPATHSEGLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsSVGElement.h"
#include "nsTArray.h"
#include "SVGPathData.h" 
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {

class DOMSVGPathSeg;
class SVGAnimatedPathSegList;


























class DOMSVGPathSegList MOZ_FINAL : public nsISupports,
                                    public nsWrapperCache
{
  friend class AutoChangePathSegListNotifier;
  friend class DOMSVGPathSeg;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPathSegList)

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

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

  



  uint32_t LengthNoFlush() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() == InternalList().CountItems(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  















  void InternalListWillChangeTo(const SVGPathData& aNewValue);

  



  bool AttrIsAnimating() const;

  uint32_t NumberOfItems() const
  {
    if (IsAnimValList()) {
      Element()->FlushAnimations();
    }
    return LengthNoFlush();
  }
  void Clear(ErrorResult& aError);
  already_AddRefed<DOMSVGPathSeg> Initialize(DOMSVGPathSeg& aNewItem,
                                             ErrorResult& aError);
  already_AddRefed<DOMSVGPathSeg> GetItem(uint32_t index,
                                          ErrorResult& error);
  already_AddRefed<DOMSVGPathSeg> IndexedGetter(uint32_t index, bool& found,
                                                ErrorResult& error);
  already_AddRefed<DOMSVGPathSeg> InsertItemBefore(DOMSVGPathSeg& aNewItem,
                                                   uint32_t aIndex,
                                                   ErrorResult& aError);
  already_AddRefed<DOMSVGPathSeg> ReplaceItem(DOMSVGPathSeg& aNewItem,
                                              uint32_t aIndex,
                                              ErrorResult& aError);
  already_AddRefed<DOMSVGPathSeg> RemoveItem(uint32_t aIndex,
                                             ErrorResult& aError);
  already_AddRefed<DOMSVGPathSeg> AppendItem(DOMSVGPathSeg& aNewItem,
                                             ErrorResult& aError)
  {
    return InsertItemBefore(aNewItem, LengthNoFlush(), aError);
  }
  uint32_t Length() const
  {
    return NumberOfItems();
  }

private:

  



  DOMSVGPathSegList(nsSVGElement *aElement, bool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    SetIsDOMBinding();

    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPathSegList();

  nsSVGElement* Element() const {
    return mElement.get();
  }

  
  bool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPathData& InternalList() const;

  SVGAnimatedPathSegList& InternalAList() const;

  
  
  already_AddRefed<DOMSVGPathSeg> GetItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex,
                                      uint32_t aInternalIndex,
                                      uint32_t aArgCountForItem);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex,
                                        int32_t aArgCountForItem);

  
  
  
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

  
  
  FallibleTArray<ItemProxy> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  bool mIsAnimValList;
};

} 

#endif 
