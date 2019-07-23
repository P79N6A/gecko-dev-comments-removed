





































#ifndef __NS_SVGTRANSFORMLIST_H__
#define __NS_SVGTRANSFORMLIST_H__

#include "nsSVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGTransformList.h"
#include "nsVoidArray.h"

class nsSVGTransformList : public nsSVGValue,
                           public nsIDOMSVGTransformList,
                           public nsISVGValueObserver
{
public:
  static nsresult Create(nsIDOMSVGTransformList** aResult);
  
protected:
  nsSVGTransformList();
  virtual ~nsSVGTransformList();
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGTRANSFORMLIST
  
  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  

  
  
  nsIDOMSVGTransform* ElementAt(PRInt32 index);
  PRBool AppendElement(nsIDOMSVGTransform* aElement);
  static already_AddRefed<nsIDOMSVGMatrix>
  GetConsolidationMatrix(nsIDOMSVGTransformList *transforms);
  
protected:
  PRInt32 ParseParameterList(char *paramstr, float *vars, PRInt32 nvars);
  void ReleaseTransforms();
  
  nsAutoVoidArray mTransforms;
};


#endif 
