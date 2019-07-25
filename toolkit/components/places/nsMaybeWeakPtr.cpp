





































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
    if (NS_SUCCEEDED(rv)) {
      return ref;
    }
  }

  return nsnull;
}

nsresult
NS_AppendWeakElementBase(isupports_array_type *aArray,
                         nsISupports *aElement,
                         bool aOwnsWeak)
{
  nsCOMPtr<nsISupports> ref;
  if (aOwnsWeak) {
    nsCOMPtr<nsIWeakReference> weakRef;
    weakRef = do_GetWeakReference(aElement);
    reinterpret_cast<nsCOMPtr<nsISupports>*>(&weakRef)->swap(ref);
  } else {
    ref = aElement;
  }

  if (aArray->IndexOf(ref) != aArray->NoIndex) {
    return NS_ERROR_INVALID_ARG; 
  }
  if (!aArray->AppendElement(ref)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult
NS_RemoveWeakElementBase(isupports_array_type *aArray,
                         nsISupports *aElement)
{
  PRUint32 index = aArray->IndexOf(aElement);
  if (index != aArray->NoIndex) {
    aArray->RemoveElementAt(index);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsISupportsWeakReference> supWeakRef = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(supWeakRef, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIWeakReference> weakRef;
  nsresult rv = supWeakRef->GetWeakReference(getter_AddRefs(weakRef));
  NS_ENSURE_SUCCESS(rv, rv);

  index = aArray->IndexOf(weakRef);
  if (index == aArray->NoIndex) {
    return NS_ERROR_INVALID_ARG;
  }

  aArray->RemoveElementAt(index);
  return NS_OK;
}
