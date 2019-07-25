






































#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozIStorageConnection.h"
#include "nsIIDBFactory.h"

#include "nsCycleCollectionParticipant.h"
#include "nsXULAppAPI.h"

class nsIAtom;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

struct DatabaseInfo;
class IDBDatabase;
struct ObjectStoreInfo;

class IDBFactory : public nsIIDBFactory
{
  typedef nsTArray<nsRefPtr<ObjectStoreInfo> > ObjectStoreInfoArray;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBFactory)
  NS_DECL_NSIIDBFACTORY

  static already_AddRefed<nsIIDBFactory> Create(nsPIDOMWindow* aWindow);

  static already_AddRefed<nsIIDBFactory> Create(JSContext* aCx,
                                                JSObject* aOwningObject);

  static already_AddRefed<mozIStorageConnection>
  GetConnection(const nsAString& aDatabaseFilePath);

  
  
  
  
  static void
  NoteUsedByProcessType(GeckoProcessType aProcessType);

  static nsresult
  GetDirectory(nsIFile** aDirectory);

  static nsresult
  GetDirectoryForOrigin(const nsACString& aASCIIOrigin,
                        nsIFile** aDirectory);

  static nsresult
  LoadDatabaseInformation(mozIStorageConnection* aConnection,
                          nsIAtom* aDatabaseId,
                          PRUint64* aVersion,
                          ObjectStoreInfoArray& aObjectStores);

  static nsresult
  SetDatabaseMetadata(DatabaseInfo* aDatabaseInfo,
                      PRUint64 aVersion,
                      ObjectStoreInfoArray& aObjectStores);

private:
  IDBFactory();
  ~IDBFactory();

  nsresult
  OpenCommon(const nsAString& aName,
             PRInt64 aVersion,
             bool aDeleting,
             nsIIDBOpenDBRequest** _retval);

  
  
  nsCOMPtr<nsPIDOMWindow> mWindow;
  JSObject* mOwningObject;
};

END_INDEXEDDB_NAMESPACE

#endif 
