





































#ifndef __NS_SVGTRANSFORM_H__
#define __NS_SVGTRANSFORM_H__

#include "nsIDOMSVGTransform.h"
#include "nsSVGValue.h"
#include "nsISVGValueObserver.h"




class nsSVGTransform : public nsIDOMSVGTransform,
                       public nsSVGValue,
                       public nsISVGValueObserver
{
public:
  static nsresult Create(nsIDOMSVGTransform** aResult);
  
protected:
  nsSVGTransform();
  ~nsSVGTransform();
  nsresult Init();
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGTRANSFORM

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

#ifdef MOZ_SMIL
  
  void GetRotationOrigin(float& aOriginX, float& aOriginY) const
  {
    aOriginX = mOriginX;
    aOriginY = mOriginY;
  }
#endif 

protected:
  nsCOMPtr<nsIDOMSVGMatrix> mMatrix;
  float mAngle, mOriginX, mOriginY;
  PRUint16 mType;
};

nsresult
NS_NewSVGTransform(nsIDOMSVGTransform** result);






#endif 
