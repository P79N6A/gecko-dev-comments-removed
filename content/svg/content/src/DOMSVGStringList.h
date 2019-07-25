




#ifndef MOZILLA_DOMSVGSTRINGLIST_H__
#define MOZILLA_DOMSVGSTRINGLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGStringList.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class SVGStringList;



























class DOMSVGStringList MOZ_FINAL : public nsIDOMSVGStringList
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGStringList)
  NS_DECL_NSIDOMSVGSTRINGLIST

  









  static already_AddRefed<DOMSVGStringList>
    GetDOMWrapper(SVGStringList *aList,
                  nsSVGElement *aElement,
                  bool aIsConditionalProcessingAttribute,
                  uint8_t aAttrEnum);

private:
  



  DOMSVGStringList(nsSVGElement *aElement,
                   bool aIsConditionalProcessingAttribute, uint8_t aAttrEnum)
    : mElement(aElement)
    , mAttrEnum(aAttrEnum)
    , mIsConditionalProcessingAttribute(aIsConditionalProcessingAttribute)
  {}

  ~DOMSVGStringList();

  SVGStringList &InternalList();

  
  nsRefPtr<nsSVGElement> mElement;

  uint8_t mAttrEnum;

  bool    mIsConditionalProcessingAttribute;
};

} 

#endif 
