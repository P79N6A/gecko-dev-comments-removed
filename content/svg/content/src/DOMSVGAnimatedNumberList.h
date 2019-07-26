




#ifndef MOZILLA_DOMSVGANIMATEDNUMBERLIST_H__
#define MOZILLA_DOMSVGANIMATEDNUMBERLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGAnimatedNumberList.h"
#include "nsSVGElement.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class DOMSVGNumberList;
class SVGAnimatedNumberList;
class SVGNumberList;
















class DOMSVGAnimatedNumberList MOZ_FINAL : public nsIDOMSVGAnimatedNumberList,
                                           public nsWrapperCache
{
  friend class DOMSVGNumberList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGAnimatedNumberList)
  NS_DECL_NSIDOMSVGANIMATEDNUMBERLIST

  










  static already_AddRefed<DOMSVGAnimatedNumberList>
    GetDOMWrapper(SVGAnimatedNumberList *aList,
                  nsSVGElement *aElement,
                  uint8_t aAttrEnum);

  




  static DOMSVGAnimatedNumberList*
    GetDOMWrapperIfExists(SVGAnimatedNumberList *aList);

  











  void InternalBaseValListWillChangeTo(const SVGNumberList& aNewValue);
  void InternalAnimValListWillChangeTo(const SVGNumberList& aNewValue);

  



  bool IsAnimating() const;

  
  nsSVGElement* GetParentObject() const { return mElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  
  already_AddRefed<DOMSVGNumberList> BaseVal();
  already_AddRefed<DOMSVGNumberList> AnimVal();

private:

  



  DOMSVGAnimatedNumberList(nsSVGElement *aElement, uint8_t aAttrEnum)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
    , mAttrEnum(aAttrEnum)
  {}

  ~DOMSVGAnimatedNumberList();

  
  SVGAnimatedNumberList& InternalAList();
  const SVGAnimatedNumberList& InternalAList() const;

  
  
  
  DOMSVGNumberList *mBaseVal;
  DOMSVGNumberList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;

  uint8_t mAttrEnum;
};

} 

#endif 
