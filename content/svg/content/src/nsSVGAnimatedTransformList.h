





































#ifndef __NS_SVGANIMATEDTRANSFORMLIST_H__
#define __NS_SVGANIMATEDTRANSFORMLIST_H__

#include "nsIDOMSVGAnimTransformList.h"
#include "nsIDOMSVGTransformList.h"
#include "nsSVGValue.h"




class nsSVGTransformSMILAttr;

class nsSVGAnimatedTransformList : public nsIDOMSVGAnimatedTransformList,
                                   public nsSVGValue,
                                   public nsISVGValueObserver
{  
protected:
  friend nsresult
  NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                                 nsIDOMSVGTransformList* baseVal);

  ~nsSVGAnimatedTransformList();
  void Init(nsIDOMSVGTransformList* baseVal);

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDTRANSFORMLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

  
  

protected:
  friend class nsSVGTransformSMILAttr;

  nsCOMPtr<nsIDOMSVGTransformList> mBaseVal;
  
  nsCOMPtr<nsIDOMSVGTransformList> mAnimVal;
};

nsresult
NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                               nsIDOMSVGTransformList* baseVal);

#endif 
