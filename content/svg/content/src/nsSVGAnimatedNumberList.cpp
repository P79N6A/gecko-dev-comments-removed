





































#include "nsSVGAnimatedNumberList.h"
#include "nsSVGNumberList.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsContentUtils.h"




class nsSVGAnimatedNumberList : public nsIDOMSVGAnimatedNumberList,
                                public nsSVGValue,
                                public nsISVGValueObserver
{  
protected:
  friend nsresult
  NS_NewSVGAnimatedNumberList(nsIDOMSVGAnimatedNumberList** result,
                              nsIDOMSVGNumberList* aBaseVal);

  nsSVGAnimatedNumberList();
  ~nsSVGAnimatedNumberList();
  nsresult Init(nsIDOMSVGNumberList* aBaseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDNUMBERLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  
  
protected:
  nsCOMPtr<nsIDOMSVGNumberList> mBaseVal;
};





nsSVGAnimatedNumberList::nsSVGAnimatedNumberList()
{
}

nsSVGAnimatedNumberList::~nsSVGAnimatedNumberList()
{
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (val)
    val->RemoveObserver(this);
}

nsresult
nsSVGAnimatedNumberList::Init(nsIDOMSVGNumberList* aBaseVal)
{
  mBaseVal = aBaseVal;
  if (!mBaseVal) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return NS_ERROR_FAILURE;
  val->AddObserver(this);
  return NS_OK;
}




NS_IMPL_ADDREF(nsSVGAnimatedNumberList)
NS_IMPL_RELEASE(nsSVGAnimatedNumberList)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedNumberList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGAnimatedNumberList::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  NS_ASSERTION(value, "value doesn't support SVGValue interfaces");
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedNumberList::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  NS_ASSERTION(value, "value doesn't support SVGValue interfaces");
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedNumberList::GetBaseVal(nsIDOMSVGNumberList * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedNumberList::GetAnimVal(nsIDOMSVGNumberList * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedNumberList::WillModifySVGObservable(nsISVGValue* observable,
                                                 modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedNumberList::DidModifySVGObservable (nsISVGValue* observable,
                                                 modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}





nsresult
NS_NewSVGAnimatedNumberList(nsIDOMSVGAnimatedNumberList** result,
                            nsIDOMSVGNumberList* aBaseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedNumberList* animatedNumberList = new nsSVGAnimatedNumberList();
  if (!animatedNumberList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedNumberList);

  if (NS_FAILED(animatedNumberList->Init(aBaseVal))) {
    NS_RELEASE(animatedNumberList);
    return NS_ERROR_FAILURE;
  }
  
  *result = (nsIDOMSVGAnimatedNumberList*) animatedNumberList;
  
  return NS_OK;
}


