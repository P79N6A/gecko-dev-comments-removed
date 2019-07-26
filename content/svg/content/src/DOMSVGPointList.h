




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
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsIDOMSVGPoint;

namespace mozilla {

class DOMSVGPoint;
class SVGAnimatedPointList;


























class DOMSVGPointList MOZ_FINAL : public nsIDOMSVGPointList,
                                  public nsWrapperCache
{
  friend class DOMSVGPoint;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPointList)
  NS_DECL_NSIDOMSVGPOINTLIST

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
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

  



  uint32_t LengthNoFlush() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() == InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  















  void InternalListWillChangeTo(const SVGPointList& aNewValue);

  



  bool AttrIsAnimating() const;

  uint32_t NumberOfItems() const
  {
    if (IsAnimValList()) {
      Element()->FlushAnimations();
    }
    return LengthNoFlush();
  }
  void Clear(ErrorResult& aError);
  already_AddRefed<nsIDOMSVGPoint> Initialize(nsIDOMSVGPoint *aNewItem,
                                              ErrorResult& aError);
  nsIDOMSVGPoint* GetItem(uint32_t aIndex, ErrorResult& aError)
  {
    bool found;
    nsIDOMSVGPoint* item = IndexedGetter(aIndex, found, aError);
    if (!found) {
      aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    }
    return item;
  }
  nsIDOMSVGPoint* IndexedGetter(uint32_t aIndex, bool& aFound,
                                ErrorResult& aError);
  already_AddRefed<nsIDOMSVGPoint> InsertItemBefore(nsIDOMSVGPoint *aNewItem,
                                                     uint32_t aIndex,
                                                     ErrorResult& aError);
  already_AddRefed<nsIDOMSVGPoint> ReplaceItem(nsIDOMSVGPoint *aNewItem,
                                               uint32_t aIndex,
                                               ErrorResult& aError);
  already_AddRefed<nsIDOMSVGPoint> RemoveItem(uint32_t aIndex,
                                              ErrorResult& aError);
  already_AddRefed<nsIDOMSVGPoint> AppendItem(nsIDOMSVGPoint *aNewItem,
                                              ErrorResult& aError)
  {
    return InsertItemBefore(aNewItem, LengthNoFlush(), aError);
  }
  uint32_t Length() const
  {
    return NumberOfItems();
  }

private:

  



  DOMSVGPointList(nsSVGElement *aElement, bool aIsAnimValList)
    : mElement(aElement)
    , mIsAnimValList(aIsAnimValList)
  {
    SetIsDOMBinding();

    InternalListWillChangeTo(InternalList()); 
  }

  ~DOMSVGPointList();

  nsSVGElement* Element() const {
    return mElement.get();
  }

  
  bool IsAnimValList() const {
    return mIsAnimValList;
  }

  







  SVGPointList& InternalList() const;

  SVGAnimatedPointList& InternalAList() const;

  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  nsTArray<DOMSVGPoint*> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  bool mIsAnimValList;
};

} 

#endif 
