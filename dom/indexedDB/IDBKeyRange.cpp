






































#include "IDBKeyRange.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE


already_AddRefed<IDBKeyRange>
IDBKeyRange::Create(nsIVariant* aLeft,
                    nsIVariant* aRight,
                    PRUint16 aFlags)
{
  nsRefPtr<IDBKeyRange> keyRange(new IDBKeyRange());
  keyRange->mLeft = aLeft;
  keyRange->mRight = aRight;
  keyRange->mFlags = aFlags;

  return keyRange.forget();
}

NS_IMPL_ADDREF(IDBKeyRange)
NS_IMPL_RELEASE(IDBKeyRange)

NS_INTERFACE_MAP_BEGIN(IDBKeyRange)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIDBKeyRange)
  NS_INTERFACE_MAP_ENTRY(nsIIDBKeyRange)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBKeyRange)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBKeyRange, IDBKeyRange)

NS_IMETHODIMP
IDBKeyRange::GetFlags(PRUint16* aFlags)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  *aFlags = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
IDBKeyRange::GetLeft(nsIVariant** aLeft)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIVariant> result(mLeft);
  result.forget(aLeft);
  return NS_OK;
}

NS_IMETHODIMP
IDBKeyRange::GetRight(nsIVariant** aRight)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIVariant> result(mRight);
  result.forget(aRight);
  return NS_OK;
}
