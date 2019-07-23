





































#include "nsSVGAnimatedLengthList.h"
#include "nsSVGLengthList.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsContentUtils.h"




class nsSVGAnimatedLengthList : public nsIDOMSVGAnimatedLengthList,
                                public nsSVGValue,
                                public nsISVGValueObserver
{  
protected:
  friend nsresult
  NS_NewSVGAnimatedLengthList(nsIDOMSVGAnimatedLengthList** result,
                              nsIDOMSVGLengthList* baseVal);

  nsSVGAnimatedLengthList();
  ~nsSVGAnimatedLengthList();
  void Init(nsIDOMSVGLengthList* baseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDLENGTHLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  
  
protected:
  nsCOMPtr<nsIDOMSVGLengthList> mBaseVal;
};





nsSVGAnimatedLengthList::nsSVGAnimatedLengthList()
{
}

nsSVGAnimatedLengthList::~nsSVGAnimatedLengthList()
{
  if (!mBaseVal) return;
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedLengthList::Init(nsIDOMSVGLengthList* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->AddObserver(this);
}




NS_IMPL_ADDREF(nsSVGAnimatedLengthList)
NS_IMPL_RELEASE(nsSVGAnimatedLengthList)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedLengthList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGAnimatedLengthList::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedLengthList::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedLengthList::GetBaseVal(nsIDOMSVGLengthList * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedLengthList::GetAnimVal(nsIDOMSVGLengthList * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedLengthList::WillModifySVGObservable(nsISVGValue* observable,
                                                 modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedLengthList::DidModifySVGObservable (nsISVGValue* observable,
                                                 modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}





nsresult
NS_NewSVGAnimatedLengthList(nsIDOMSVGAnimatedLengthList** result,
                            nsIDOMSVGLengthList* baseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedLengthList* animatedLengthList = new nsSVGAnimatedLengthList();
  if(!animatedLengthList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedLengthList);

  animatedLengthList->Init(baseVal);
  
  *result = (nsIDOMSVGAnimatedLengthList*) animatedLengthList;
  
  return NS_OK;
}

