






































#include "AsyncDatabaseConnection.h"

#include "nsIIDBObjectStore.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIVariant.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCID.h"

#include "IDBEvents.h"

USING_INDEXEDDB_NAMESPACE

namespace {

PRUint32 gConnectionCount = 0;
nsIThread* gDatabaseThread = nsnull;
PRBool gXPCOMShutdown = PR_FALSE;

struct ObjectStore
{
  ObjectStore() : autoIncrement(PR_FALSE), readerCount(0), writing(PR_FALSE) { }

  nsString name;
  nsString keyPath;
  PRBool autoIncrement;

  nsTArray<nsString> mIndexes;
  PRUint32 readerCount;
  PRBool writing;
};

nsTArray<ObjectStore>* gObjectStores = nsnull;

class ShutdownObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  Observe(nsISupports* aSubject,
          const char* aTopic,
          const PRUnichar* aData)
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(!gXPCOMShutdown, "Already shutdown?!");

    if (gDatabaseThread) {
      nsresult rv = gDatabaseThread->Shutdown();
      NS_RELEASE(gDatabaseThread);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    delete gObjectStores;
    gObjectStores = nsnull;

    gXPCOMShutdown = PR_TRUE;
    return NS_OK;
  }
};

already_AddRefed<nsIDOMEvent>
MakeSuccessEventBool(PRBool aResult)
{
  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(variant, nsnull);

  nsresult rv = variant->SetAsBool(aResult);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = variant->SetWritable(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCOMPtr<nsIDOMEvent> event(IDBSuccessEvent::Create(variant));
  return event.forget();
}

already_AddRefed<nsIDOMEvent>
MakeSuccessEvent(nsISupports* aResult)
{
  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(variant, nsnull);

  nsresult rv = variant->SetAsISupports(aResult);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = variant->SetWritable(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCOMPtr<nsIDOMEvent> event(IDBSuccessEvent::Create(variant));
  return event.forget();
}

class DatabaseWorkHelper : public nsRunnable
{
public:
  static const PRUint16 OK = PR_UINT16_MAX;

  DatabaseWorkHelper(nsIDOMEventTarget* aTarget)
  : mTarget(aTarget), mError(PR_FALSE), mErrorCode(0)
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  }

  virtual ~DatabaseWorkHelper()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(!mTarget, "Badness!");
  }

  NS_IMETHOD
  Run()
  {
    if (NS_IsMainThread()) {
      if (mError) {
        
        nsCOMPtr<nsIDOMEvent> event(IDBErrorEvent::Create(mErrorCode));
        PRBool dummy;
        mTarget->DispatchEvent(event, &dummy);
      }
      else {
        OnSuccess();
      }

      mTarget = nsnull;
      return NS_OK;
    }

    NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

    
    {
      nsCOMPtr<nsIDOMEventTarget> target;
      target.swap(mTarget);

      mErrorCode = DoDatabaseWork();

      target.swap(mTarget);
    }

    mError = mErrorCode != OK;

    NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
    return NS_OK;
  }

  nsresult
  Dispatch()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    if (!gDatabaseThread) {
      return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
    }
    return gDatabaseThread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

protected:
  virtual PRUint16 DoDatabaseWork() = 0;
  virtual void OnSuccess() = 0;

  nsCOMPtr<nsIDOMEventTarget> mTarget;

private:
  PRBool mError;
  PRUint16 mErrorCode;
};

class CreateObjectStoreHelper : public DatabaseWorkHelper
{
public:
  CreateObjectStoreHelper(const nsAString& aName,
                          const nsAString& aKeyPath,
                          PRBool aAutoIncrement,
                          nsIDOMEventTarget* aTarget)
  : DatabaseWorkHelper(aTarget), mName(aName), mKeyPath(aKeyPath),
    mAutoIncrement(aAutoIncrement)
  { }

  PRUint16
  DoDatabaseWork()
  {
    PRUint32 count = gObjectStores->Length();
    for (PRUint32 index = 0; index < count; index++) {
      if (gObjectStores->ElementAt(index).name.Equals(mName)) {
        return nsIIDBDatabaseError::CONSTRAINT_ERR;
      }
    }

    ObjectStore* newStore = gObjectStores->AppendElement();
    newStore->name.Assign(mName);
    newStore->keyPath.Assign(mKeyPath);
    newStore->autoIncrement = mAutoIncrement;

    return OK;
  }

  void
  OnSuccess()
  {
    nsCOMPtr<nsIDOMEvent> event(MakeSuccessEventBool(PR_TRUE));
    PRBool dummy;
    mTarget->DispatchEvent(event, &dummy);
  }

protected:
  nsString mName;
  nsString mKeyPath;
  PRBool mAutoIncrement;
};

class OpenObjectStoreHelper : public DatabaseWorkHelper
{
public:
  OpenObjectStoreHelper(const nsAString& aName,
                        PRUint16 aMode,
                        nsIDOMEventTarget* aTarget)
  : DatabaseWorkHelper(aTarget), mName(aName), mMode(aMode)
  { }

  PRUint16
  DoDatabaseWork()
  {
    PRUint32 count = gObjectStores->Length();
    for (PRUint32 index = 0; index < count; index++) {
      ObjectStore& store = gObjectStores->ElementAt(index);
      if (store.name.Equals(mName)) {
        if (mMode == nsIIDBObjectStore::READ_WRITE) {
          store.writing = PR_TRUE;
        }
        store.readerCount++;
        return OK;
      }
    }

    return nsIIDBDatabaseError::NOT_FOUND_ERR;
  }

  void
  OnSuccess()
  {
    nsCOMPtr<nsIDOMEvent> event(MakeSuccessEventBool(PR_TRUE));
    PRBool dummy;
    mTarget->DispatchEvent(event, &dummy);
  }

protected:
  nsString mName;
  PRUint16 mMode;
};

} 

NS_IMPL_ISUPPORTS1(ShutdownObserver, nsIObserver);


AsyncDatabaseConnection*
AsyncDatabaseConnection::OpenConnection(const nsAString& aName,
                                        const nsAString& aDescription,
                                        PRBool aReadOnly)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  nsAutoPtr<AsyncDatabaseConnection> connection(new AsyncDatabaseConnection());

  if (!gConnectionCount++) {
    if (gXPCOMShutdown) {
      NS_ERROR("Creating a new connection after shutdown!");
      return nsnull;
    }

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    NS_ENSURE_TRUE(obs, nsnull);

    nsCOMPtr<nsIObserver> observer(new ShutdownObserver());
    nsresult rv = obs->AddObserver(observer, "xpcom-shutdown-threads",
                                   PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);

    NS_ASSERTION(!gObjectStores, "Already got a store?!");
    gObjectStores = new nsAutoTArray<ObjectStore, 10>;

    rv = NS_NewThread(&gDatabaseThread);
    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  NS_ASSERTION(gDatabaseThread, "No thread?!");
  return connection.forget();
}

AsyncDatabaseConnection::AsyncDatabaseConnection()
: mReadOnly(PR_FALSE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

AsyncDatabaseConnection::~AsyncDatabaseConnection()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

nsresult
AsyncDatabaseConnection::CreateObjectStore(const nsAString& aName,
                                           const nsAString& aKeyPath,
                                           PRBool aAutoIncrement,
                                           nsIDOMEventTarget* aTarget)
{
  nsRefPtr<CreateObjectStoreHelper> helper =
    new CreateObjectStoreHelper(aName, aKeyPath, aAutoIncrement, aTarget);
  return helper->Dispatch();
}

nsresult
AsyncDatabaseConnection::OpenObjectStore(const nsAString& aName,
                                         PRUint16 aMode,
                                         nsIDOMEventTarget* aTarget)
{
  nsRefPtr<OpenObjectStoreHelper> helper =
    new OpenObjectStoreHelper(aName, aMode, aTarget);
  return helper->Dispatch();
}
