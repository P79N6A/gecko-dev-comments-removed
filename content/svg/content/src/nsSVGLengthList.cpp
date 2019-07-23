





































#include "nsSVGLengthList.h"
#include "nsSVGLength.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsVoidArray.h"
#include "nsDOMError.h"
#include "nsReadableUtils.h"
#include "nsSVGSVGElement.h"
#include "nsCRT.h"
#include "nsISVGValueUtils.h"
#include "nsContentUtils.h"




class nsSVGLengthList : public nsIDOMSVGLengthList,
                        public nsSVGValue,
                        public nsISVGValueObserver
{  
protected:
  friend nsresult NS_NewSVGLengthList(nsIDOMSVGLengthList** result,
                                      nsSVGElement *aContext,
                                      PRUint8 aCtxType);

  nsSVGLengthList(nsSVGElement *aContext, PRUint8 aCtxType);
  ~nsSVGLengthList();

  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGLENGTHLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  

protected:
  
  nsISVGLength* ElementAt(PRInt32 index);
  void AppendElement(nsISVGLength* aElement);
  void RemoveElementAt(PRInt32 index);
  void InsertElementAt(nsISVGLength* aElement, PRInt32 index);
  
  void ReleaseLengths();
  
  nsAutoVoidArray mLengths;
  nsWeakPtr mContext;  
  PRUint8 mCtxType;
};





nsSVGLengthList::nsSVGLengthList(nsSVGElement *aContext, PRUint8 aCtxType)
  : mCtxType(aCtxType)
{
  mContext = do_GetWeakReference(static_cast<nsGenericElement*>(aContext));
}

nsSVGLengthList::~nsSVGLengthList()
{
  ReleaseLengths();
}




NS_IMPL_ADDREF(nsSVGLengthList)
NS_IMPL_RELEASE(nsSVGLengthList)

NS_INTERFACE_MAP_BEGIN(nsSVGLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLengthList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGLengthList::SetValueString(const nsAString& aValue)
{
  WillModify();
  
  ReleaseLengths();

  nsresult rv = NS_OK;

  char* str;
  str = ToNewCString(aValue);

  char* rest = str;
  char* token;
  const char* delimiters = ",\x20\x9\xD\xA";

  while ((token = nsCRT::strtok(rest, delimiters, &rest))) {
    nsCOMPtr<nsISVGLength> length;
    NS_NewSVGLength(getter_AddRefs(length), NS_ConvertASCIItoUTF16(token));
    if (!length) {
      rv = NS_ERROR_FAILURE;
      break;
    }
    AppendElement(length);
  }
  
  nsMemory::Free(str);
  
  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGLengthList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRInt32 count = mLengths.Count();

  if (count<=0) return NS_OK;

  PRInt32 i = 0;
  
  while (1) {
    nsISVGLength* length = ElementAt(i);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(length);
    NS_ASSERTION(val, "length doesn't implement required interface");
    if (!val) continue;
    nsAutoString str;
    val->GetValueString(str);
    aValue.Append(str);

    if (++i >= count) break;

    aValue.AppendLiteral(" ");
  }
  
  return NS_OK;
}





NS_IMETHODIMP nsSVGLengthList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mLengths.Count();
  return NS_OK;
}


NS_IMETHODIMP nsSVGLengthList::Clear()
{
  WillModify();
  ReleaseLengths();
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGLengthList::Initialize(nsIDOMSVGLength *newItem,
                                          nsIDOMSVGLength **_retval)
{
  if (!newItem) {
    *_retval = nsnull;
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  Clear();
  return AppendItem(newItem, _retval);
}


NS_IMETHODIMP nsSVGLengthList::GetItem(PRUint32 index, nsIDOMSVGLength **_retval)
{
  if (index >= static_cast<PRUint32>(mLengths.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = ElementAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGLengthList::InsertItemBefore(nsIDOMSVGLength *newItem,
                                  PRUint32 index,
                                  nsIDOMSVGLength **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGLengthList::InsertItemBefore");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGLengthList::ReplaceItem(nsIDOMSVGLength *newItem,
                             PRUint32 index,
                             nsIDOMSVGLength **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("nsSVGLengthList::ReplaceItem");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGLengthList::RemoveItem(PRUint32 index, nsIDOMSVGLength **_retval)
{
  if (index >= static_cast<PRUint32>(mLengths.Count())) {
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
nsSVGLengthList::AppendItem(nsIDOMSVGLength *newItem, nsIDOMSVGLength **_retval)
{
  nsCOMPtr<nsISVGLength> length = do_QueryInterface(newItem);
  if (!length) {
    *_retval = nsnull;
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  AppendElement(length);

  *_retval = newItem;
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGLengthList::WillModifySVGObservable(nsISVGValue* observable,
                                         modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGLengthList::DidModifySVGObservable(nsISVGValue* observable,
                                        modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




void
nsSVGLengthList::ReleaseLengths()
{
  WillModify();
  PRInt32 count = mLengths.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsISVGLength* length = ElementAt(i);
    length->SetContext(nsnull, 0);
    NS_REMOVE_SVGVALUE_OBSERVER(length);
    NS_RELEASE(length);
  }
  mLengths.Clear();
  DidModify();
}

nsISVGLength*
nsSVGLengthList::ElementAt(PRInt32 index)
{
  return (nsISVGLength*)mLengths.ElementAt(index);
}

void
nsSVGLengthList::AppendElement(nsISVGLength* aElement)
{
  WillModify();
  NS_ADDREF(aElement);
  
  
  
  
  
  
  aElement->SetContext(mContext, mCtxType);
  mLengths.AppendElement((void*)aElement);
  NS_ADD_SVGVALUE_OBSERVER(aElement);
  DidModify();
}

void
nsSVGLengthList::RemoveElementAt(PRInt32 index)
{
  WillModify();
  nsISVGLength* length = ElementAt(index);
  NS_ASSERTION(length, "null length");
  NS_REMOVE_SVGVALUE_OBSERVER(length);
  mLengths.RemoveElementAt(index);
  NS_RELEASE(length);
  DidModify();
}

void
nsSVGLengthList::InsertElementAt(nsISVGLength* aElement, PRInt32 index)
{
  WillModify();
  NS_ADDREF(aElement);

  
  
  
  
  
  aElement->SetContext(mContext, mCtxType);
  
  mLengths.InsertElementAt((void*)aElement, index);
  NS_ADD_SVGVALUE_OBSERVER(aElement);
  DidModify();
}





nsresult
NS_NewSVGLengthList(nsIDOMSVGLengthList** result, nsSVGElement *aContext, PRUint8 aCtxType)
{
  *result = nsnull;
  
  nsSVGLengthList* lengthList = new nsSVGLengthList(aContext, aCtxType);
  if (!lengthList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(lengthList);

  *result = lengthList;
  
  return NS_OK;
}

