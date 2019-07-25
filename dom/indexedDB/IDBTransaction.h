






































#ifndef mozilla_dom_indexeddb_idbtransaction_h__
#define mozilla_dom_indexeddb_idbtransaction_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"
#include "mozilla/dom/indexedDB/FileInfo.h"

#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "nsIIDBTransaction.h"
#include "nsIRunnable.h"
#include "nsIThreadInternal.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class CommitHelper;
struct ObjectStoreInfo;
class TransactionThreadPool;
class UpdateRefcountFunction;

class IDBTransactionListener
{
public:
  NS_IMETHOD_(nsrefcnt) AddRef() = 0;
  NS_IMETHOD_(nsrefcnt) Release() = 0;

  virtual nsresult NotifyTransactionComplete(IDBTransaction* aTransaction) = 0;
};

class IDBTransaction : public nsDOMEventTargetHelper,
                       public nsIIDBTransaction,
                       public nsIThreadObserver
{
  friend class AsyncConnectionHelper;
  friend class CommitHelper;
  friend class ThreadObserver;
  friend class TransactionThreadPool;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBTRANSACTION
  NS_DECL_NSITHREADOBSERVER

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBTransaction,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBTransaction>
  Create(IDBDatabase* aDatabase,
         nsTArray<nsString>& aObjectStoreNames,
         PRUint16 aMode,
         bool aDispatchDelayed);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  void OnNewRequest();
  void OnRequestFinished();

  void RemoveObjectStore(const nsAString& aName);

  void SetTransactionListener(IDBTransactionListener* aListener);

  bool StartSavepoint();
  nsresult ReleaseSavepoint();
  void RollbackSavepoint();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

  already_AddRefed<mozIStorageStatement>
  GetCachedStatement(const nsACString& aQuery);

  template<int N>
  already_AddRefed<mozIStorageStatement>
  GetCachedStatement(const char (&aQuery)[N])
  {
    return GetCachedStatement(NS_LITERAL_CSTRING(aQuery));
  }

  bool IsOpen() const;

  bool IsWriteAllowed() const
  {
    return mMode == nsIIDBTransaction::READ_WRITE ||
           mMode == nsIIDBTransaction::VERSION_CHANGE;
  }

  bool IsAborted() const
  {
    return mAborted;
  }

  PRUint16 Mode()
  {
    return mMode;
  }

  IDBDatabase* Database()
  {
    NS_ASSERTION(mDatabase, "This should never be null!");
    return mDatabase;
  }

  DatabaseInfo* DBInfo() const
  {
    return mDatabaseInfo;
  }

  already_AddRefed<IDBObjectStore>
  GetOrCreateObjectStore(const nsAString& aName,
                         ObjectStoreInfo* aObjectStoreInfo);

  void OnNewFileInfo(FileInfo* aFileInfo);

  void ClearCreatedFileInfos();

private:
  IDBTransaction();
  ~IDBTransaction();

  nsresult CommitOrRollback();

  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<DatabaseInfo> mDatabaseInfo;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mPendingRequests;
  PRUint32 mCreatedRecursionDepth;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;

  nsInterfaceHashtable<nsCStringHashKey, mozIStorageStatement>
    mCachedStatements;

  nsRefPtr<IDBTransactionListener> mListener;

  
  nsCOMPtr<mozIStorageConnection> mConnection;

  
  PRUint32 mSavepointCount;

  nsTArray<nsRefPtr<IDBObjectStore> > mCreatedObjectStores;

  bool mAborted;
  bool mCreating;

#ifdef DEBUG
  bool mFiredCompleteOrAbort;
#endif

  nsRefPtr<UpdateRefcountFunction> mUpdateFileRefcountFunction;
  nsTArray<nsRefPtr<FileInfo> > mCreatedFileInfos;
};

class CommitHelper : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  CommitHelper(IDBTransaction* aTransaction,
               IDBTransactionListener* aListener,
               const nsTArray<nsRefPtr<IDBObjectStore> >& mUpdatedObjectStores);
  ~CommitHelper();

  template<class T>
  bool AddDoomedObject(nsCOMPtr<T>& aCOMPtr)
  {
    if (aCOMPtr) {
      if (!mDoomedObjects.AppendElement(do_QueryInterface(aCOMPtr))) {
        NS_ERROR("Out of memory!");
        return false;
      }
      aCOMPtr = nsnull;
    }
    return true;
  }

private:
  
  nsresult WriteAutoIncrementCounts();

  
  void CommitAutoIncrementCounts();

  
  void RevertAutoIncrementCounts();

  nsRefPtr<IDBTransaction> mTransaction;
  nsRefPtr<IDBTransactionListener> mListener;
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsRefPtr<UpdateRefcountFunction> mUpdateFileRefcountFunction;
  nsAutoTArray<nsCOMPtr<nsISupports>, 10> mDoomedObjects;
  nsAutoTArray<nsRefPtr<IDBObjectStore>, 10> mAutoIncrementObjectStores;

  bool mAborted;
};

class UpdateRefcountFunction : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  UpdateRefcountFunction(FileManager* aFileManager)
  : mFileManager(aFileManager)
  { }

  ~UpdateRefcountFunction()
  { }

  nsresult Init();

  void ClearFileInfoEntries()
  {
    mFileInfoEntries.Clear();
  }

  nsresult UpdateDatabase(mozIStorageConnection* aConnection)
  {
    DatabaseUpdateFunction function(aConnection);

    mFileInfoEntries.EnumerateRead(DatabaseUpdateCallback, &function);

    return function.ErrorCode();
  }

  void UpdateFileInfos()
  {
    mFileInfoEntries.EnumerateRead(FileInfoUpdateCallback, nsnull);
  }

private:
  class FileInfoEntry
  {
  public:
    FileInfoEntry(FileInfo* aFileInfo)
    : mFileInfo(aFileInfo), mDelta(0)
    { }

    ~FileInfoEntry()
    { }

    nsRefPtr<FileInfo> mFileInfo;
    PRInt32 mDelta;
  };

  enum UpdateType {
    eIncrement,
    eDecrement
  };

  class DatabaseUpdateFunction
  {
  public:
    DatabaseUpdateFunction(mozIStorageConnection* aConnection)
    : mConnection(aConnection), mErrorCode(NS_OK)
    { }

    bool Update(PRInt64 aId, PRInt32 aDelta);
    nsresult ErrorCode()
    {
      return mErrorCode;
    }

  private:
    nsresult UpdateInternal(PRInt64 aId, PRInt32 aDelta);

    nsCOMPtr<mozIStorageConnection> mConnection;
    nsCOMPtr<mozIStorageStatement> mUpdateStatement;
    nsCOMPtr<mozIStorageStatement> mInsertStatement;

    nsresult mErrorCode;
  };

  nsresult ProcessValue(mozIStorageValueArray* aValues,
                        PRInt32 aIndex,
                        UpdateType aUpdateType);

  static PLDHashOperator
  DatabaseUpdateCallback(const PRUint64& aKey,
                         FileInfoEntry* aValue,
                         void* aUserArg);

  static PLDHashOperator
  FileInfoUpdateCallback(const PRUint64& aKey,
                         FileInfoEntry* aValue,
                         void* aUserArg);

  FileManager* mFileManager;
  nsClassHashtable<nsUint64HashKey, FileInfoEntry> mFileInfoEntries;
};

END_INDEXEDDB_NAMESPACE

#endif 
