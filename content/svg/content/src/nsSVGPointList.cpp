





































#include "nsSVGPointList.h"
#include "nsSVGPoint.h"
#include "nsSVGUtils.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsDOMError.h"
#include "prdtoa.h"
#include "nsReadableUtils.h"
#include "nsTextFormatter.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsContentUtils.h"

#define NS_ENSURE_NATIVE_POINT(obj, retval)             \
  {                                                     \
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(obj); \
    if (!val) {                                         \
      *retval = nsnull;                                 \
      return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;           \
    }                                                   \
  }

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
  PRUint32 count = mPoints.Length();
  for (PRUint32 i = 0; i < count; ++i) {
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
  return mPoints.ElementAt(index);
}

void
nsSVGPointList::AppendElement(nsIDOMSVGPoint* aElement)
{
  WillModify();
  NS_ADDREF(aElement);
  mPoints.AppendElement(aElement);
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
  mPoints.InsertElementAt(index, aElement);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
  if (val)
    val->AddObserver(this);
  DidModify();
}




NS_IMPL_ADDREF(nsSVGPointList)
NS_IMPL_RELEASE(nsSVGPointList)

DOMCI_DATA(SVGPointList, nsSVGPointList)

NS_INTERFACE_MAP_BEGIN(nsSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPointList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGPointList::SetValueString(const nsAString& aValue)
{
  nsCharSeparatedTokenizer
    tokenizer(aValue, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  nsCOMArray<nsIDOMSVGPoint> points;

  PRBool parseError = PR_FALSE;

  while (tokenizer.hasMoreTokens()) {
    
    NS_ConvertUTF16toUTF8 utf8String1(tokenizer.nextToken());
    const char *token1 = utf8String1.get();
    if (!tokenizer.hasMoreTokens() ||  
        *token1 == '\0') {             
      parseError = PR_TRUE;
      break;
    }
    NS_ConvertUTF16toUTF8 utf8String2(tokenizer.nextToken());
    const char *token2 = utf8String2.get();
    if (*token2 == '\0') {             
      parseError = PR_TRUE;
      break;
    }

    
    char *end;
    float x = float(PR_strtod(token1, &end));
    if (*end != '\0' || !NS_FloatIsFinite(x)) {
      parseError = PR_TRUE;
      break;
    }
    float y = float(PR_strtod(token2, &end));
    if (*end != '\0' || !NS_FloatIsFinite(y)) {
      parseError = PR_TRUE;
      break;
    }

    
    nsCOMPtr<nsIDOMSVGPoint> point;
    NS_NewSVGPoint(getter_AddRefs(point), x, y); 
    points.AppendObject(point);
  }

  if (tokenizer.lastTokenEndedWithSeparator()) { 
    parseError = PR_TRUE;
  }

  if (parseError) {
    
  }

  WillModify();
  ReleasePoints();
  PRInt32 count = points.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    AppendElement(points.ObjectAt(i));
  }
  DidModify();

  return NS_OK;
}

NS_IMETHODIMP
nsSVGPointList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRUint32 count = mPoints.Length();

  if (count == 0) return NS_OK;

  PRUint32 i = 0;
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
  *aNumberOfItems = mPoints.Length();
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
  NS_ENSURE_NATIVE_POINT(newItem, _retval);
  Clear();
  return AppendItem(newItem, _retval);
}


NS_IMETHODIMP nsSVGPointList::GetItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if (index >= mPoints.Length()) {
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
  NS_ENSURE_NATIVE_POINT(newItem, _retval);

  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGPointList::ReplaceItem(nsIDOMSVGPoint *newItem,
                                          PRUint32 index,
                                          nsIDOMSVGPoint **_retval)
{
  NS_ENSURE_NATIVE_POINT(newItem, _retval);

  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGPointList::RemoveItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if (index >= mPoints.Length()) {
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
  
  
  
  
  NS_ENSURE_NATIVE_POINT(newItem, _retval);
  *_retval = newItem;
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
