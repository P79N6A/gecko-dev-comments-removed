






































#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozIStorageConnection.h"
#include "nsIIDBFactory.h"

#include "nsIWeakReferenceUtils.h"
#include "nsXULAppAPI.h"

class nsPIDOMWindow;
class nsIAtom;

BEGIN_INDEXEDDB_NAMESPACE

struct DatabaseInfo;
class IDBDatabase;
struct ObjectStoreInfo;

class IDBFactory : public nsIIDBFactory
{
  typedef nsTArray<nsAutoPtr<ObjectStoreInfo> > ObjectStoreInfoArray;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBFACTORY

  static already_AddRefed<nsIIDBFactory> Create(nsPIDOMWindow* aWindow);

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
  UpdateDatabaseMetadata(DatabaseInfo* aDatabaseInfo,
                         PRUint64 aVersion,
                         ObjectStoreInfoArray& aObjectStores);

private:
  IDBFactory();
  ~IDBFactory() { }

  nsCOMPtr<nsIWeakReference> mWindow;
};

END_INDEXEDDB_NAMESPACE

#endif 
