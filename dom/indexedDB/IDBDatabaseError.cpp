






































#include "IDBDatabaseError.h"

#include "nsDOMClassInfo.h"
#include "nsDOMClassInfoID.h"

USING_INDEXEDDB_NAMESPACE

namespace {

void
GetMessageForErrorCode(PRUint16 aCode,
                       nsAString& aMessage)
{
  switch (aCode) {
    case nsIIDBDatabaseError::NON_TRANSIENT_ERR:
      aMessage.AssignLiteral("This error occurred because an operation was not "
                             "allowed on an object. A retry of the same "
                             "operation would fail unless the cause of the "
                             "error is corrected.");
      break;
    case nsIIDBDatabaseError::NOT_FOUND_ERR:
      aMessage.AssignLiteral("The operation failed because the requested "
                             "database object could not be found. For example, "
                             "an object store did not exist but was being "
                             "opened.");
      break;
    case nsIIDBDatabaseError::CONSTRAINT_ERR:
      aMessage.AssignLiteral("A mutation operation in the transaction failed "
                             "due to a because a constraint was not satisfied. "
                             "For example, an object such as an object store "
                             "or index already exists and a new one was being "
                             "attempted to be created.");
      break;
    case nsIIDBDatabaseError::DATA_ERR:
      aMessage.AssignLiteral("Data provided to an operation does not meet "
                             "requirements.");
      break;
    case nsIIDBDatabaseError::NOT_ALLOWED_ERR:
      aMessage.AssignLiteral("A mutation operation was attempted on a database "
                             "that did not allow mutations.");
      break;
    case nsIIDBDatabaseError::SERIAL_ERR:
      aMessage.AssignLiteral("The operation failed because of the size of the "
                             "data set being returned or because there was a "
                             "problem in serializing or deserializing the "
                             "object being processed.");
      break;
    case nsIIDBDatabaseError::RECOVERABLE_ERR:
      aMessage.AssignLiteral("The operation failed because the database was "
                             "prevented from taking an action. The operation "
                             "might be able to succeed if the application "
                             "performs some recovery steps and retries the "
                             "entire transaction. For example, there was not "
                             "enough remaining storage space, or the storage "
                             "quota was reached and the user declined to give "
                             "more space to the database.");
      break;
    case nsIIDBDatabaseError::TRANSIENT_ERR:
      aMessage.AssignLiteral("The operation failed because of some temporary "
                             "problems. The failed operation might be able to "
                             "succeed when the operation is retried without "
                             "any intervention by application-level "
                             "functionality.");
      break;
    case nsIIDBDatabaseError::TIMEOUT_ERR:
      aMessage.AssignLiteral("A lock for the transaction could not be obtained "
                             "in a reasonable time.");
      break;
    case nsIIDBDatabaseError::DEADLOCK_ERR:
      aMessage.AssignLiteral("The current transaction was automatically rolled "
                             "back by the database becuase of deadlock or "
                             "other transaction serialization failures.");
      break;
    case nsIIDBDatabaseError::UNKNOWN_ERR:
      
    default:
      aMessage.AssignLiteral("The operation failed for reasons unrelated to "
                             "the database itself and not covered by any other "
                             "error code.");
  }
}

} 

IDBDatabaseError::IDBDatabaseError(PRUint16 aCode)
: mCode(aCode)
{
  GetMessageForErrorCode(mCode, mMessage);
}

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
