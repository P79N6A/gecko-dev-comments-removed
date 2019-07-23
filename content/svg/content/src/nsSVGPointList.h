





































#ifndef __NS_SVGPOINTLIST_H__
#define __NS_SVGPOINTLIST_H__

#include "nsSVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGPointList.h"
#include "nsVoidArray.h"


class nsSVGPointList : public nsSVGValue,
                       public nsIDOMSVGPointList,
                       public nsISVGValueObserver
{
public:
  static nsresult Create(const nsAString& aValue, nsISVGValue** aResult);
  static nsresult Create(nsIDOMSVGPointList** aResult);
  
protected:
  nsSVGPointList();
  virtual ~nsSVGPointList();
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPOINTLIST
  
  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  

  
  
  nsIDOMSVGPoint* ElementAt(PRInt32 index);
  void AppendElement(nsIDOMSVGPoint* aElement);
  void RemoveElementAt(PRInt32 index);
  void InsertElementAt(nsIDOMSVGPoint* aElement, PRInt32 index);
  
protected:
  void ReleasePoints();
  
  nsAutoVoidArray mPoints;
};


#endif 
