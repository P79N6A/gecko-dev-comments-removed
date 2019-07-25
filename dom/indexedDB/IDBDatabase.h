






































#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIDocument.h"
#include "nsIIDBDatabase.h"

#include "mozilla/dom/indexedDB/IDBWrapperCache.h"
#include "mozilla/dom/indexedDB/FileManager.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBIndex;
class IDBObjectStore;
class IDBTransaction;
class IndexedDatabaseManager;

class IDBDatabase : public IDBWrapperCache,
                    public nsIIDBDatabase
{
  friend class AsyncConnectionHelper;
  friend class IndexedDatabaseManager;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBDATABASE

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBDatabase, IDBWrapperCache)

  static already_AddRefed<IDBDatabase>
  Create(IDBWrapperCache* aOwnerCache,
         already_AddRefed<DatabaseInfo> aDatabaseInfo,
         const nsACString& aASCIIOrigin,
         FileManager* aFileManager);

  
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  nsIAtom* Id() const
  {
    return mDatabaseId;
  }

  DatabaseInfo* Info() const
  {
    return mDatabaseInfo;
  }

  const nsString& Name()
  {
    return mName;
  }

  const nsString& FilePath()
  {
    return mFilePath;
  }

  already_AddRefed<nsIDocument> GetOwnerDocument()
  {
    if (!mOwner) {
      return nsnull;
    }

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mOwner->GetExtantDocument());
    return doc.forget();
  }

  nsCString& Origin()
  {
    return mASCIIOrigin;
  }

  void Invalidate();

  
  
  bool IsInvalidated();

  void CloseInternal(bool aIsDead);

  
  bool IsClosed();

  void EnterSetVersionTransaction();
  void ExitSetVersionTransaction();

  FileManager* Manager() const
  {
    return mFileManager;
  }

private:
  IDBDatabase();
  ~IDBDatabase();

  void OnUnlink();

  nsRefPtr<DatabaseInfo> mDatabaseInfo;
  nsCOMPtr<nsIAtom> mDatabaseId;
  nsString mName;
  nsString mFilePath;
  nsCString mASCIIOrigin;

  PRInt32 mInvalidated;
  bool mRegistered;
  bool mClosed;
  bool mRunningVersionChange;

  nsRefPtr<FileManager> mFileManager;

  
  NS_DECL_EVENT_HANDLER(abort)
  NS_DECL_EVENT_HANDLER(error)
  NS_DECL_EVENT_HANDLER(versionchange)
};

END_INDEXEDDB_NAMESPACE

#endif
