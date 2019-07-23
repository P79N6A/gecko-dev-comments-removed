





































#include "nsSVGPathSegList.h"
#include "nsSVGPathSeg.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "nsDOMError.h"
#include "nsSVGPathDataParser.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"




class nsSVGPathSegList : public nsIDOMSVGPathSegList,
                         public nsSVGValue,
                         public nsISVGValueObserver
{  
protected:
  friend nsresult NS_NewSVGPathSegList(nsIDOMSVGPathSegList** result);

  nsSVGPathSegList();
  ~nsSVGPathSegList();


public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLIST

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  

protected:
  
  void AppendElement(nsSVGPathSeg* aElement);
  void RemoveElementAt(PRInt32 index);
  void InsertElementAt(nsSVGPathSeg* aElement, PRInt32 index);
  void RemoveFromCurrentList(nsSVGPathSeg*);

  void ReleaseSegments(PRBool aModify = PR_TRUE);

  nsCOMArray<nsIDOMSVGPathSeg> mSegments;
};





nsSVGPathSegList::nsSVGPathSegList()
{
}

nsSVGPathSegList::~nsSVGPathSegList()
{
  PRInt32 count = mSegments.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsSVGPathSeg* seg = NS_STATIC_CAST(nsSVGPathSeg*, mSegments.ObjectAt(i));
    seg->SetCurrentList(nsnull);
  }
}




NS_IMPL_ADDREF(nsSVGPathSegList)
NS_IMPL_RELEASE(nsSVGPathSegList)

NS_INTERFACE_MAP_BEGIN(nsSVGPathSegList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPathSegList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPathSegList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGPathSegList::SetValueString(const nsAString& aValue)
{
  nsresult rv;
  
  WillModify();
  ReleaseSegments(PR_FALSE);
  nsSVGPathDataParserToDOM parser(&mSegments);
  rv = parser.Parse(aValue);

  PRInt32 count = mSegments.Count();
  for (PRInt32 i=0; i<count; ++i) {
    nsSVGPathSeg* seg = NS_STATIC_CAST(nsSVGPathSeg*, mSegments.ObjectAt(i));
    seg->SetCurrentList(this);
  }

  if (NS_FAILED(rv)) {
    NS_ERROR("path data parse error!");
    ReleaseSegments(PR_FALSE);
  }
  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGPathSegList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRInt32 count = mSegments.Count();

  if (count<=0) return NS_OK;

  PRInt32 i = 0;

  while (1) {
    nsSVGPathSeg* seg = NS_STATIC_CAST(nsSVGPathSeg*, mSegments.ObjectAt(i));

    nsAutoString str;
    seg->GetValueString(str);
    aValue.Append(str);

    if (++i >= count) break;

    aValue.AppendLiteral(" ");
  }

  return NS_OK;
}





NS_IMETHODIMP nsSVGPathSegList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mSegments.Count();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::Clear()
{
  WillModify();
  ReleaseSegments();
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::Initialize(nsIDOMSVGPathSeg *newItem,
                                           nsIDOMSVGPathSeg **_retval)
{
  NS_ENSURE_NATIVE_PATH_SEG(newItem, _retval);
  Clear();
  return AppendItem(newItem, _retval);
}


NS_IMETHODIMP nsSVGPathSegList::GetItem(PRUint32 index, nsIDOMSVGPathSeg **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mSegments.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = mSegments.ObjectAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::InsertItemBefore(nsIDOMSVGPathSeg *newItem,
                                                 PRUint32 index,
                                                 nsIDOMSVGPathSeg **_retval)
{
  NS_ENSURE_NATIVE_PATH_SEG(newItem, _retval);

  if (index >= NS_STATIC_CAST(PRUint32, mSegments.Count())) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  InsertElementAt(NS_STATIC_CAST(nsSVGPathSeg*, newItem), index);
  NS_ADDREF(*_retval = newItem);

  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::ReplaceItem(nsIDOMSVGPathSeg *newItem,
                                            PRUint32 index,
                                            nsIDOMSVGPathSeg **_retval)
{
  NS_ENSURE_NATIVE_PATH_SEG(newItem, _retval);

  if (index >= NS_STATIC_CAST(PRUint32, mSegments.Count())) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  InsertElementAt(NS_STATIC_CAST(nsSVGPathSeg*, newItem), index);
  RemoveElementAt(index+1);
  NS_ADDREF(*_retval = newItem);

  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::RemoveItem(PRUint32 index, nsIDOMSVGPathSeg **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mSegments.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval = mSegments.ObjectAt(index);
  NS_ADDREF(*_retval);
  WillModify();
  RemoveElementAt(index);
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegList::AppendItem(nsIDOMSVGPathSeg *newItem,
                                           nsIDOMSVGPathSeg **_retval)
{
  NS_ENSURE_NATIVE_PATH_SEG(newItem, _retval);
  NS_ADDREF(*_retval = newItem);
  AppendElement(NS_STATIC_CAST(nsSVGPathSeg*, newItem));
  return NS_OK;
}





NS_IMETHODIMP
nsSVGPathSegList::WillModifySVGObservable(nsISVGValue* observable,
                                          modificationType aModType)
{
  

  NS_NOTYETIMPLEMENTED("nsSVGPathSegList::WillModifySVGObservable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGPathSegList::DidModifySVGObservable (nsISVGValue* observable,
                                          modificationType aModType)
{
  

  NS_NOTYETIMPLEMENTED("nsSVGPathSegList::DidModifySVGObservable");
  return NS_ERROR_NOT_IMPLEMENTED;
}




void
nsSVGPathSegList::ReleaseSegments(PRBool aModify)
{
  if (aModify) {
    WillModify();
  }
  PRInt32 count = mSegments.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsSVGPathSeg* seg = NS_STATIC_CAST(nsSVGPathSeg*, mSegments.ObjectAt(i));
    seg->SetCurrentList(nsnull);
  }
  mSegments.Clear();
  if (aModify) {
    DidModify();
  }
}

void
nsSVGPathSegList::AppendElement(nsSVGPathSeg* aElement)
{
  WillModify();
  
  
  RemoveFromCurrentList(aElement);
  mSegments.AppendObject(aElement);
  aElement->SetCurrentList(this);
  DidModify();
}

void
nsSVGPathSegList::RemoveElementAt(PRInt32 index)
{
  WillModify();
  nsSVGPathSeg* seg = NS_STATIC_CAST(nsSVGPathSeg*, mSegments.ObjectAt(index));
  seg->SetCurrentList(nsnull);
  mSegments.RemoveObjectAt(index);
  DidModify();
}

void
nsSVGPathSegList::InsertElementAt(nsSVGPathSeg* aElement, PRInt32 index)
{
  WillModify();
  
  
  RemoveFromCurrentList(aElement);
  mSegments.InsertObjectAt(aElement, index);
  aElement->SetCurrentList(this);
  DidModify();
}




void 
nsSVGPathSegList::RemoveFromCurrentList(nsSVGPathSeg* aSeg)
{
  nsCOMPtr<nsISVGValue> currentList = aSeg->GetCurrentList();
  if (currentList) {
    
    nsSVGPathSegList* otherSegList = NS_STATIC_CAST(nsSVGPathSegList*, currentList.get());
    PRInt32 ix = otherSegList->mSegments.IndexOfObject(aSeg);
    if (ix != -1) { 
      otherSegList->RemoveElementAt(ix); 
    }
    aSeg->SetCurrentList(nsnull);
  }
}




nsresult
NS_NewSVGPathSegList(nsIDOMSVGPathSegList** result)
{
  *result = nsnull;
  
  nsSVGPathSegList* pathSegList = new nsSVGPathSegList();
  if (!pathSegList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(pathSegList);

  

  *result = (nsIDOMSVGPathSegList*) pathSegList;

  return NS_OK;
}

