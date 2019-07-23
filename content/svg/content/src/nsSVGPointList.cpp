





































#include "nsSVGPointList.h"
#include "nsSVGPoint.h"
#include "nsDOMError.h"
#include "prdtoa.h"
#include "nsReadableUtils.h"
#include "nsTextFormatter.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsContentUtils.h"

nsresult
nsSVGPointList::Create(const nsAString& aValue,
                       nsISVGValue** aResult)
{
  *aResult = (nsISVGValue*) new nsSVGPointList();
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);

  (*aResult)->SetValueString(aValue);  
  return NS_OK;
}

nsresult
nsSVGPointList::Create(nsIDOMSVGPointList** aResult)
{
  *aResult = (nsIDOMSVGPointList*) new nsSVGPointList();
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsSVGPointList::nsSVGPointList()
{
}

nsSVGPointList::~nsSVGPointList()
{
  ReleasePoints();
}

void
nsSVGPointList::ReleasePoints()
{
  WillModify();
  PRInt32 count = mPoints.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsIDOMSVGPoint* point = ElementAt(i);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(point);
    if (val)
      val->RemoveObserver(this);
    NS_RELEASE(point);
  }
  mPoints.Clear();
  DidModify();
}

nsIDOMSVGPoint*
nsSVGPointList::ElementAt(PRInt32 index)
{
  return (nsIDOMSVGPoint*)mPoints.ElementAt(index);
}

void
nsSVGPointList::AppendElement(nsIDOMSVGPoint* aElement)
{
  WillModify();
  NS_ADDREF(aElement);
  mPoints.AppendElement((void*)aElement);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
  if (val)
    val->AddObserver(this);
  DidModify();
}

void
nsSVGPointList::RemoveElementAt(PRInt32 index)
{
  WillModify();
  nsIDOMSVGPoint* point = ElementAt(index);
  NS_ASSERTION(point, "null point");
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(point);
  if (val)
    val->RemoveObserver(this);
  mPoints.RemoveElementAt(index);
  NS_RELEASE(point);
  DidModify();
}

void
nsSVGPointList::InsertElementAt(nsIDOMSVGPoint* aElement, PRInt32 index)
{
  WillModify();
  NS_ADDREF(aElement);
  mPoints.InsertElementAt((void*)aElement, index);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
  if (val)
    val->AddObserver(this);
  DidModify();
}




NS_IMPL_ADDREF(nsSVGPointList)
NS_IMPL_RELEASE(nsSVGPointList)

NS_INTERFACE_MAP_BEGIN(nsSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPointList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGPointList::SetValueString(const nsAString& aValue)
{
  nsresult rv = NS_OK;

  char* str = ToNewCString(aValue);
  
  char* rest = str;
  char* token1;
  char* token2;
  const char* delimiters = ",\x20\x9\xD\xA";
  nsCOMArray<nsIDOMSVGPoint> points;
  
  while ( (token1 = nsCRT::strtok(rest, delimiters, &rest)) &&
          (token2 = nsCRT::strtok(rest, delimiters, &rest)) ) {

    char *end;
    
    double x = PR_strtod(token1, &end);
    if (*end != '\0') {
      rv = NS_ERROR_FAILURE;
      break; 
    }
    double y = PR_strtod(token2, &end);
    if (*end != '\0') {
      rv = NS_ERROR_FAILURE;
      break; 
    }
    
    nsCOMPtr<nsIDOMSVGPoint> point;
    NS_NewSVGPoint(getter_AddRefs(point), (float)x, (float)y);
    if (!point) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    }
    points.AppendObject(point);
  }

  if (token1 || NS_FAILED(rv)) {
    
    rv = NS_ERROR_FAILURE;
  }
  else {
    WillModify();
    ReleasePoints();
    PRInt32 count = points.Count();
    for (PRInt32 i=0; i<count; ++i) {
      AppendElement(points.ObjectAt(i));
    }
    DidModify();
  }

  nsMemory::Free(str);
  
  return rv;
}

NS_IMETHODIMP
nsSVGPointList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRInt32 count = mPoints.Count();

  if (count<=0) return NS_OK;

  PRInt32 i = 0;
  PRUnichar buf[48];
  
  while (1) {
    nsIDOMSVGPoint* point = ElementAt(i);
    float x, y;
    point->GetX(&x);
    point->GetY(&y);
    
    nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar), NS_LITERAL_STRING("%g,%g").get(), (double)x, (double)y);
    aValue.Append(buf);

    if (++i >= count) break;

    aValue.AppendLiteral(" ");
  }
  
  return NS_OK;
}





NS_IMETHODIMP nsSVGPointList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mPoints.Count();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPointList::Clear()
{
  WillModify();
  ReleasePoints();
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPointList::Initialize(nsIDOMSVGPoint *newItem,
                                         nsIDOMSVGPoint **_retval)
{
  if (!newItem) {
    *_retval = nsnull;
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  Clear();
  return AppendItem(newItem, _retval);
}


NS_IMETHODIMP nsSVGPointList::GetItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mPoints.Count())) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = ElementAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGPointList::InsertItemBefore(nsIDOMSVGPoint *newItem,
                                               PRUint32 index,
                                               nsIDOMSVGPoint **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGPointList::ReplaceItem(nsIDOMSVGPoint *newItem,
                                          PRUint32 index,
                                          nsIDOMSVGPoint **_retval)
{
  
  
  

  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGPointList::RemoveItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if (index >= NS_STATIC_CAST(PRUint32, mPoints.Count())) {
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


NS_IMETHODIMP nsSVGPointList::AppendItem(nsIDOMSVGPoint *newItem,
                                         nsIDOMSVGPoint **_retval)
{
  
  
  
  
  *_retval = newItem;
  if (!newItem)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  AppendElement(newItem);
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGPointList::WillModifySVGObservable(nsISVGValue* observable,
                                        modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGPointList::DidModifySVGObservable (nsISVGValue* observable,
                                        modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}
