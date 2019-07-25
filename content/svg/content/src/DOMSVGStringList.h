




































#ifndef MOZILLA_DOMSVGSTRINGLIST_H__
#define MOZILLA_DOMSVGSTRINGLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGStringList.h"
#include "nsSVGElement.h"

namespace mozilla {

class SVGStringList;



























class DOMSVGStringList : public nsIDOMSVGStringList
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGStringList)
  NS_DECL_NSIDOMSVGSTRINGLIST

  









  static already_AddRefed<DOMSVGStringList>
    GetDOMWrapper(SVGStringList *aList,
                  nsSVGElement *aElement,
                  bool aIsConditionalProcessingAttribute,
                  PRUint8 aAttrEnum);

private:
  



  DOMSVGStringList(nsSVGElement *aElement,
                   bool aIsConditionalProcessingAttribute, PRUint8 aAttrEnum)
    : mElement(aElement)
    , mAttrEnum(aAttrEnum)
    , mIsConditionalProcessingAttribute(aIsConditionalProcessingAttribute)
  {}

  ~DOMSVGStringList();

  void DidChangeStringList(PRUint8 aAttrEnum, bool aDoSetAttr);

  SVGStringList &InternalList();

  
  nsRefPtr<nsSVGElement> mElement;

  PRUint8 mAttrEnum;

  bool    mIsConditionalProcessingAttribute;
};

} 

#endif 
