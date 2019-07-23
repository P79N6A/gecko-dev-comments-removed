





































#include "nsMaybeWeakPtr.h"

void*
nsMaybeWeakPtr_base::GetValueAs(const nsIID &iid) const
{
  nsresult rv;
  void *ref;
  if (mPtr) {
    rv = mPtr->QueryInterface(iid, &ref);
    if (NS_SUCCEEDED(rv)) {
      return ref;
    }
  }

  nsCOMPtr<nsIWeakReference> weakRef = do_QueryInterface(mPtr);
  if (weakRef) {
    rv = weakRef->QueryReferent(iid, &ref);
    if (NS_FAILED(rv)) {
      ref = nsnull;
    }
  }
  return ref;
}

 nsresult
nsMaybeWeakPtrArray_base::AppendWeakElementBase(nsTArray_base *aArray,
                                                nsISupports *aElement,
                                                PRBool aOwnsWeak)
{
  nsCOMPtr<nsISupports> ref;
  if (aOwnsWeak) {
    nsCOMPtr<nsIWeakReference> weakRef;
    weakRef = do_GetWeakReference(aElement);
    reinterpret_cast<nsCOMPtr<nsISupports>*>(&weakRef)->swap(ref);
  } else {
    ref = aElement;
  }

  isupports_type *array = static_cast<isupports_type*>(aArray);
  if (array->IndexOf(ref) != isupports_type::NoIndex) {
    return NS_ERROR_INVALID_ARG; 
  }
  if (!array->AppendElement(ref)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

 nsresult
nsMaybeWeakPtrArray_base::RemoveWeakElementBase(nsTArray_base *aArray,
                                                nsISupports *aElement)
{
  isupports_type *array = static_cast<isupports_type*>(aArray);
  PRUint32 index = array->IndexOf(aElement);
  if (index != isupports_type::NoIndex) {
    array->RemoveElementAt(index);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsISupportsWeakReference> supWeakRef = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(supWeakRef, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIWeakReference> weakRef;
  nsresult rv = supWeakRef->GetWeakReference(getter_AddRefs(weakRef));
  NS_ENSURE_SUCCESS(rv, rv);

  index = array->IndexOf(weakRef);
  if (index == isupports_type::NoIndex) {
    return NS_ERROR_INVALID_ARG;
  }

  array->RemoveElementAt(index);
  return NS_OK;
}
