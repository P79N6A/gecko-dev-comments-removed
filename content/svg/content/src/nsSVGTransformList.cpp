






































#include "nsSVGTransformList.h"
#include "nsSVGTransformListParser.h"
#include "nsSVGTransform.h"
#include "nsSVGMatrix.h"
#include "nsDOMError.h"
#include "prdtoa.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsContentUtils.h"
#include "nsIDOMClassInfo.h"

#define NS_ENSURE_NATIVE_TRANSFORM(obj, retval)                    \
  {                                                                \
    nsresult rv;                                                   \
    if (retval)                                                    \
      *retval = nsnull;                                            \
    nsCOMPtr<nsISVGValue> transform = do_QueryInterface(obj, &rv); \
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);        \
  }

nsresult
nsSVGTransformList::Create(nsIDOMSVGTransformList** aResult)
{
  *aResult = new nsSVGTransformList();
  if (!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsSVGTransformList::nsSVGTransformList()
{
}

nsSVGTransformList::~nsSVGTransformList()
{
  ReleaseTransforms();
}

void
nsSVGTransformList::ReleaseTransforms()
{
  PRUint32 count = mTransforms.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIDOMSVGTransform* transform = ElementAt(i);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(transform);
    val->RemoveObserver(this);
    NS_RELEASE(transform);
  }
  mTransforms.Clear();
}

nsIDOMSVGTransform*
nsSVGTransformList::ElementAt(PRInt32 index)
{
  return mTransforms.ElementAt(index);
}

PRBool
nsSVGTransformList::AppendElement(nsIDOMSVGTransform* aElement)
{
  if (mTransforms.AppendElement(aElement)) {
    NS_ADDREF(aElement);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
    val->AddObserver(this);
    return PR_TRUE;
  }

  return PR_FALSE;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGTransformList::GetConsolidationMatrix(nsIDOMSVGTransformList *transforms)
{
  PRUint32 count;
  transforms->GetNumberOfItems(&count);

  if (!count)
    return nsnull;

  nsCOMPtr<nsIDOMSVGTransform> transform;
  nsCOMPtr<nsIDOMSVGMatrix> conmatrix;

  
  if (count == 1) {
    transforms->GetItem(0, getter_AddRefs(transform));
    transform->GetMatrix(getter_AddRefs(conmatrix));
  } else {
    nsresult rv = NS_NewSVGMatrix(getter_AddRefs(conmatrix));
    NS_ENSURE_SUCCESS(rv, nsnull);
    
    nsCOMPtr<nsIDOMSVGMatrix> temp1, temp2;
    
    for (PRUint32 i = 0; i < count; ++i) {
      transforms->GetItem(i, getter_AddRefs(transform));
      transform->GetMatrix(getter_AddRefs(temp1));
      conmatrix->Multiply(temp1, getter_AddRefs(temp2));
      if (!temp2)
        return nsnull;
      conmatrix = temp2;
    }
  }

  nsIDOMSVGMatrix *_retval = nsnull;
  conmatrix.swap(_retval);
  return _retval;
}




NS_IMPL_ADDREF(nsSVGTransformList)
NS_IMPL_RELEASE(nsSVGTransformList)

NS_INTERFACE_MAP_BEGIN(nsSVGTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTransformList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGTransformList::SetValueString(const nsAString& aValue)
{
  
  

  nsresult rv = NS_OK;

  
  nsCOMArray<nsIDOMSVGTransform> xforms;
  nsSVGTransformListParser parser(&xforms);
  rv = parser.Parse(aValue);

  if (NS_FAILED(rv)) {
    
    rv = NS_ERROR_DOM_SYNTAX_ERR;
  }
  else {
    WillModify();
    ReleaseTransforms();
    PRInt32 count = xforms.Count();
    for (PRInt32 i=0; i<count; ++i) {
      AppendElement(xforms.ObjectAt(i));
    }
    DidModify();
  }

  return rv;
}

NS_IMETHODIMP
nsSVGTransformList::GetValueString(nsAString& aValue)
{
  aValue.Truncate();

  PRUint32 count = mTransforms.Length();

  if (count == 0) return NS_OK;

  PRUint32 i = 0;
  
  while (1) {
    nsIDOMSVGTransform* transform = ElementAt(i);

    nsCOMPtr<nsISVGValue> val = do_QueryInterface(transform);
    nsAutoString str;
    val->GetValueString(str);
    aValue.Append(str);

    if (++i >= count) break;

    aValue.AppendLiteral(" ");
  }
  
  return NS_OK;

}





NS_IMETHODIMP nsSVGTransformList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mTransforms.Length();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::Clear()
{
  WillModify();
  ReleaseTransforms();
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::Initialize(nsIDOMSVGTransform *newItem,
                                             nsIDOMSVGTransform **_retval)
{
  NS_ENSURE_NATIVE_TRANSFORM(newItem, _retval);

  nsSVGValueAutoNotifier autonotifier(this);

  ReleaseTransforms();
  if (!AppendElement(newItem)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *_retval = newItem;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::GetItem(PRUint32 index, nsIDOMSVGTransform **_retval)
{
  if (index >= mTransforms.Length()) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = ElementAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::InsertItemBefore(nsIDOMSVGTransform *newItem,
                                                   PRUint32 index,
                                                   nsIDOMSVGTransform **_retval)
{
  NS_ENSURE_NATIVE_TRANSFORM(newItem, _retval);

  nsSVGValueAutoNotifier autonotifier(this);

  PRUint32 count = mTransforms.Length();

  if (!mTransforms.InsertElementAt((index < count)? index: count, newItem)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(newItem);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(newItem);
  val->AddObserver(this);

  *_retval = newItem;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::ReplaceItem(nsIDOMSVGTransform *newItem,
                                              PRUint32 index,
                                              nsIDOMSVGTransform **_retval)
{
  NS_ENSURE_NATIVE_TRANSFORM(newItem, _retval);

  nsSVGValueAutoNotifier autonotifier(this);

  if (index >= mTransforms.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  nsIDOMSVGTransform* oldItem = ElementAt(index);

  mTransforms.ElementAt(index) = newItem;

  nsCOMPtr<nsISVGValue> val = do_QueryInterface(oldItem);
  val->RemoveObserver(this);
  NS_RELEASE(oldItem);
  val = do_QueryInterface(newItem);
  val->AddObserver(this);
  NS_ADDREF(newItem);

  *_retval = newItem;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::RemoveItem(PRUint32 index, nsIDOMSVGTransform **_retval)
{
  nsSVGValueAutoNotifier autonotifier(this);

  if (index >= mTransforms.Length()) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval = ElementAt(index);

  mTransforms.RemoveElementAt(index);

  nsCOMPtr<nsISVGValue> val = do_QueryInterface(*_retval);
  val->RemoveObserver(this);

  
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::AppendItem(nsIDOMSVGTransform *newItem,
                                             nsIDOMSVGTransform **_retval)
{
  NS_ENSURE_NATIVE_TRANSFORM(newItem, _retval);

  nsSVGValueAutoNotifier autonotifier(this);

  if (!AppendElement(newItem)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *_retval = newItem;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTransformList::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix,
                                                 nsIDOMSVGTransform **_retval)
{
  NS_ENSURE_NATIVE_MATRIX(matrix, _retval);

  nsresult rv = NS_NewSVGTransform(_retval);
  if (NS_FAILED(rv))
    return rv;

  (*_retval)->SetMatrix(matrix);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransformList::Consolidate(nsIDOMSVGTransform **_retval)
{
  

  *_retval = nsnull;

  PRUint32 count = mTransforms.Length();
  if (count==0) return NS_OK;
  if (count==1) {
    *_retval = ElementAt(0);
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMSVGMatrix> conmatrix = GetConsolidationMatrix(this);
  if (!conmatrix)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIDOMSVGTransform> consolidation;
  nsresult rv = CreateSVGTransformFromMatrix(conmatrix,
                                             getter_AddRefs(consolidation));
  if (NS_FAILED(rv))
    return rv;

  ReleaseTransforms();
  if (!AppendElement(consolidation))
    return NS_ERROR_OUT_OF_MEMORY;

  *_retval = consolidation;
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGTransformList::WillModifySVGObservable(nsISVGValue* observable,
                                            modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGTransformList::DidModifySVGObservable (nsISVGValue* observable,
                                            modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}
