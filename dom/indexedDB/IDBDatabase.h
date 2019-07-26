





#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIDocument.h"
#include "nsIFileStorage.h"
#include "nsIIDBDatabase.h"
#include "nsIOfflineStorage.h"

#include "nsDOMEventTargetHelper.h"

#include "mozilla/dom/indexedDB/FileManager.h"
#include "mozilla/dom/indexedDB/IDBWrapperCache.h"

class nsIScriptContext;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
class ContentParent;
namespace quota {
class Client;
}
}
}

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBFactory;
class IDBIndex;
class IDBObjectStore;
class IDBTransaction;
class IndexedDatabaseManager;
class IndexedDBDatabaseChild;
class IndexedDBDatabaseParent;
struct ObjectStoreInfoGuts;

class IDBDatabase : public IDBWrapperCache,
                    public nsIIDBDatabase,
                    public nsIOfflineStorage
{
  friend class AsyncConnectionHelper;
  friend class IndexedDatabaseManager;
  friend class IndexedDBDatabaseChild;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIFILESTORAGE
  NS_DECL_NSIOFFLINESTORAGE_NOCLOSE

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBDatabase, IDBWrapperCache)

  static already_AddRefed<IDBDatabase>
  Create(IDBWrapperCache* aOwnerCache,
         IDBFactory* aFactory,
         already_AddRefed<DatabaseInfo> aDatabaseInfo,
         const nsACString& aASCIIOrigin,
         FileManager* aFileManager,
         mozilla::dom::ContentParent* aContentParent);

  static IDBDatabase*
  FromStorage(nsIOfflineStorage* aStorage);

  static IDBDatabase*
  FromStorage(nsIFileStorage* aStorage)
  {
    nsCOMPtr<nsIOfflineStorage> storage = do_QueryInterface(aStorage);
    return storage ? FromStorage(storage) : nullptr;
  }

  
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  DatabaseInfo* Info() const
  {
    return mDatabaseInfo;
  }

  const nsString& Name() const
  {
    return mName;
  }

  const nsString& FilePath() const
  {
    return mFilePath;
  }

  already_AddRefed<nsIDocument> GetOwnerDocument()
  {
    if (!GetOwner()) {
      return nullptr;
    }

    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(GetOwner()->GetExtantDocument());
    return doc.forget();
  }

  void DisconnectFromActorParent();

  void CloseInternal(bool aIsDead);

  void EnterSetVersionTransaction();
  void ExitSetVersionTransaction();

  
  
  void RevertToPreviousState();

  FileManager* Manager() const
  {
    return mFileManager;
  }

  void
  SetActor(IndexedDBDatabaseChild* aActorChild)
  {
    NS_ASSERTION(!aActorChild || !mActorChild, "Shouldn't have more than one!");
    mActorChild = aActorChild;
  }

  void
  SetActor(IndexedDBDatabaseParent* aActorParent)
  {
    NS_ASSERTION(!aActorParent || !mActorParent,
                 "Shouldn't have more than one!");
    mActorParent = aActorParent;
  }

  IndexedDBDatabaseChild*
  GetActorChild() const
  {
    return mActorChild;
  }

  IndexedDBDatabaseParent*
  GetActorParent() const
  {
    return mActorParent;
  }

  mozilla::dom::ContentParent*
  GetContentParent() const
  {
    return mContentParent;
  }

  nsresult
  CreateObjectStoreInternal(IDBTransaction* aTransaction,
                            const ObjectStoreInfoGuts& aInfo,
                            IDBObjectStore** _retval);

private:
  IDBDatabase();
  ~IDBDatabase();

  void OnUnlink();

  
  
  
  nsRefPtr<IDBFactory> mFactory;

  nsRefPtr<DatabaseInfo> mDatabaseInfo;

  
  
  nsRefPtr<DatabaseInfo> mPreviousDatabaseInfo;
  nsCOMPtr<nsIAtom> mDatabaseId;
  nsString mName;
  nsString mFilePath;
  nsCString mASCIIOrigin;

  nsRefPtr<FileManager> mFileManager;

  IndexedDBDatabaseChild* mActorChild;
  IndexedDBDatabaseParent* mActorParent;

  mozilla::dom::ContentParent* mContentParent;

  nsRefPtr<mozilla::dom::quota::Client> mQuotaClient;

  bool mInvalidated;
  bool mRegistered;
  bool mClosed;
  bool mRunningVersionChange;
};

END_INDEXEDDB_NAMESPACE

#endif 
