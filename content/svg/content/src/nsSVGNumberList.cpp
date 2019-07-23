





































#include "nsSVGNumberList.h"
#include "nsSVGNumber.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsVoidArray.h"
#include "nsDOMError.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsISVGValueUtils.h"
#include "prdtoa.h"
#include "nsContentUtils.h"




class nsSVGNumberList : public nsIDOMSVGNumberList,
                        public nsSVGValue,
                        public nsISVGValueObserver
{  
protected:
  friend nsresult NS_NewSVGNumberList(nsIDOMSVGNumberList** result);

  nsSVGNumberList();
  ~nsSVGNumberList();

  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGNUMBERLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

  
  
  
protected:
  
  nsIDOMSVGNumber* ElementAt(PRInt32 index);
  void AppendElement(nsIDOMSVGNumber* aElement);
  void RemoveElementAt(PRInt32 index);
  nsresult InsertElementAt(nsIDOMSVGNumber* aElement, PRInt32 index);
  
  void ReleaseNumbers();
  
  nsAutoVoidArray mNumbers;
};





nsSVGNumberList::nsSVGNumberList()
{
}

nsSVGNumberList::~nsSVGNumberList()
{
  ReleaseNumbers();
}




NS_IMPL_ADDREF(nsSVGNumberList)
NS_IMPL_RELEASE(nsSVGNumberList)

NS_INTERFACE_MAP_BEGIN(nsSVGNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGNumberList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGNumberList::SetValueString(const nsAString& aValue)
{
  WillModify();
  ReleaseNumbers();

  nsresult rv = NS_OK;

  char* str;
  str = ToNewCString(aValue);

  char* rest = str;
  char* token;
  const char* delimiters = ", \t\r\n";

  while ((token = nsCRT::strtok(rest, delimiters, &rest))) {
    char *left;
    double value = PR_strtod(token, &left);
    if (token!=left) {
      nsCOMPtr<nsIDOMSVGNumber> number;
      NS_NewSVGNumber(getter_AddRefs(number), float(value));
      if (!number) {
        rv = NS_ERROR_FAILURE;
        break;
      }
      AppendElement(number);
    }
  }
  
  nsMemory::Free(str);
  
  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGNumberList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRInt32 count = mNumbers.Count();

  if (count<=0) return NS_OK;

  PRInt32 i = 0;
  
  while (1) {
    nsIDOMSVGNumber* number = ElementAt(i);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(number);
    NS_ASSERTION(val, "number doesn't implement required interface");
    if (!val) continue;
    nsAutoString str;
    val->GetValueString(str);
    aValue.Append(str);

    if (++i >= count) break;

    aValue.AppendLiteral(" ");
  }
  
  return NS_OK;
}





NS_IMETHODIMP nsSVGNumberList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mNumbers.Count();
  return NS_OK;
}


NS_IMETHODIMP nsSVGNumberList::Clear()
{
  WillModify();
  ReleaseNumbers();
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGNumberList::Initialize(nsIDOMSVGNumber *newItem,
                                          nsIDOMSVGNumber **_retval)
{
  if (!newItem) {
    *_retval = nsnull;
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  Clear();
  return AppendItem(newItem, _retval);
}


NS_IMETHODIMP nsSVGNumberList::GetItem(PRUint32 index, nsIDOMSVGNumber **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mNumbers.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = ElementAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGNumberList::InsertItemBefore(nsIDOMSVGNumber *newItem,
                                  PRUint32 index,
                                  nsIDOMSVGNumber **_retval)
{
  *_retval = newItem;
  if (!newItem)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsSVGValueAutoNotifier autonotifier(this);

  PRInt32 idx = index;
  PRInt32 count = mNumbers.Count();

  if (!InsertElementAt(newItem, (idx < count)? idx: count)) {
    *_retval = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGNumberList::ReplaceItem(nsIDOMSVGNumber *newItem,
                             PRUint32 index,
                             nsIDOMSVGNumber **_retval)
{
  if (!newItem) {
    *_retval = nsnull;
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }

  nsresult rv = RemoveItem(index, _retval);
  if (NS_FAILED(rv))
    return rv;

  return InsertElementAt(newItem, index);
}


NS_IMETHODIMP nsSVGNumberList::RemoveItem(PRUint32 index, nsIDOMSVGNumber **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mNumbers.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval = ElementAt(index);
  NS_ADDREF(*_retval);
  WillModify();
  RemoveElementAt(index);
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGNumberList::AppendItem(nsIDOMSVGNumber *newItem, nsIDOMSVGNumber **_retval)
{
  *_retval = newItem;
  if (!newItem)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  AppendElement(newItem);
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGNumberList::WillModifySVGObservable(nsISVGValue* observable,
                                         modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGNumberList::DidModifySVGObservable(nsISVGValue* observable,
                                        modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




void
nsSVGNumberList::ReleaseNumbers()
{
  WillModify();
  PRInt32 count = mNumbers.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsIDOMSVGNumber* number = ElementAt(i);
    NS_REMOVE_SVGVALUE_OBSERVER(number);
    NS_RELEASE(number);
  }
  mNumbers.Clear();
  DidModify();
}

nsIDOMSVGNumber*
nsSVGNumberList::ElementAt(PRInt32 index)
{
  return (nsIDOMSVGNumber*)mNumbers.ElementAt(index);
}

void
nsSVGNumberList::AppendElement(nsIDOMSVGNumber* aElement)
{
  WillModify();
  NS_ADDREF(aElement);
  
  
  
  
  
  
  mNumbers.AppendElement((void*)aElement);
  NS_ADD_SVGVALUE_OBSERVER(aElement);
  DidModify();
}

void
nsSVGNumberList::RemoveElementAt(PRInt32 index)
{
  WillModify();
  nsIDOMSVGNumber* number = ElementAt(index);
  NS_ASSERTION(number, "null number");
  NS_REMOVE_SVGVALUE_OBSERVER(number);
  mNumbers.RemoveElementAt(index);
  NS_RELEASE(number);
  DidModify();
}

nsresult
nsSVGNumberList::InsertElementAt(nsIDOMSVGNumber* aElement, PRInt32 index)
{
  nsresult rv;
  WillModify();
  NS_ADDREF(aElement);

  
  
  
  
  
  if (!NS_FAILED(rv = mNumbers.InsertElementAt((void*)aElement, index)))
    NS_ADD_SVGVALUE_OBSERVER(aElement);
  DidModify();
  return rv;
}





nsresult
NS_NewSVGNumberList(nsIDOMSVGNumberList** result)
{
  *result = nsnull;
  
  nsSVGNumberList* numberList = new nsSVGNumberList();
  if (!numberList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(numberList);

  *result = numberList;
  
  return NS_OK;
}


