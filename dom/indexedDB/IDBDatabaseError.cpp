






































#include "IDBDatabaseError.h"

#include "nsDOMClassInfo.h"
#include "nsDOMClassInfoID.h"

USING_INDEXEDDB_NAMESPACE

NS_IMPL_ADDREF(IDBDatabaseError)
NS_IMPL_RELEASE(IDBDatabaseError)

NS_INTERFACE_MAP_BEGIN(IDBDatabaseError)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIDBDatabaseError)
  NS_INTERFACE_MAP_ENTRY(nsIIDBDatabaseError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBDatabaseError)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBDatabaseError, IDBDatabaseError)

NS_IMETHODIMP
IDBDatabaseError::GetCode(PRUint16* aCode)
{
  *aCode = mCode;
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseError::SetCode(PRUint16 aCode)
{
  mCode = aCode;
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseError::GetMessage(nsAString& aMessage)
{
  aMessage.Assign(mMessage);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseError::SetMessage(const nsAString& aMessage)
{
  mMessage.Assign(aMessage);
  return NS_OK;
}
