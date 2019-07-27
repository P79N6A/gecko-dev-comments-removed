





#ifndef mozilla_dom_DataStoreDB_h
#define mozilla_dom_DataStoreDB_h

#include "mozilla/dom/IDBTransactionBinding.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventListener.h"
#include "nsISupportsImpl.h"
#include "nsString.h"

#define DATASTOREDB_REVISION       "revision"

namespace mozilla {
namespace dom {

namespace indexedDB {
class IDBDatabase;
class IDBFactory;
class IDBOpenDBRequest;
class IDBTransaction;
}

class DataStoreDBCallback;

class DataStoreDB final : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS

  DataStoreDB(const nsAString& aManifestURL, const nsAString& aName);

  nsresult Open(IDBTransactionMode aMode, const Sequence<nsString>& aDb,
                DataStoreDBCallback* aCallback);

  nsresult Delete();

  indexedDB::IDBTransaction* Transaction() const;

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;

private:
  ~DataStoreDB();

  nsresult CreateFactoryIfNeeded();

  nsresult UpgradeSchema(nsIDOMEvent* aEvent);

  nsresult DatabaseOpened();

  nsresult AddEventListeners();

  nsresult RemoveEventListeners();

  nsString mDatabaseName;

  nsRefPtr<indexedDB::IDBFactory> mFactory;
  nsRefPtr<indexedDB::IDBOpenDBRequest> mRequest;
  nsRefPtr<indexedDB::IDBDatabase> mDatabase;
  nsRefPtr<indexedDB::IDBTransaction> mTransaction;

  nsRefPtr<DataStoreDBCallback> mCallback;

  
  enum StateType {
    Inactive,
    Active
  } mState;

  IDBTransactionMode mTransactionMode;
  Sequence<nsString> mObjectStores;
  bool mCreatedSchema;
};

} 
} 

#endif 
