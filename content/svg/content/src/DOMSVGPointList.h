




#ifndef MOZILLA_DOMSVGPOINTLIST_H__
#define MOZILLA_DOMSVGPOINTLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsSVGElement.h"
#include "nsTArray.h"
#include "SVGPointList.h" 
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {

class DOMSVGPoint;
class nsISVGPoint;
class SVGAnimatedPointList;


























class DOMSVGPointList MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
  friend class nsISVGPoint;
  friend class mozilla::DOMSVGPoint;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPointList)

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

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
  already_AddRefed<nsISVGPoint> Initialize(nsISVGPoint& aNewItem,
                                           ErrorResult& aError);
  nsISVGPoint* GetItem(uint32_t aIndex, ErrorResult& aError)
  {
    bool found;
    nsISVGPoint* item = IndexedGetter(aIndex, found, aError);
    if (!found) {
      aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    }
    return item;
  }
  nsISVGPoint* IndexedGetter(uint32_t aIndex, bool& aFound,
                             ErrorResult& aError);
  already_AddRefed<nsISVGPoint> InsertItemBefore(nsISVGPoint& aNewItem,
                                                 uint32_t aIndex,
                                                 ErrorResult& aError);
  already_AddRefed<nsISVGPoint> ReplaceItem(nsISVGPoint& aNewItem,
                                            uint32_t aIndex,
                                            ErrorResult& aError);
  already_AddRefed<nsISVGPoint> RemoveItem(uint32_t aIndex,
                                           ErrorResult& aError);
  already_AddRefed<nsISVGPoint> AppendItem(nsISVGPoint& aNewItem,
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

  
  
  nsTArray<nsISVGPoint*> mItems;

  
  
  nsRefPtr<nsSVGElement> mElement;

  bool mIsAnimValList;
};

} 

#endif 
