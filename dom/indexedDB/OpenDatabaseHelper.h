



#ifndef mozilla_dom_indexeddb_opendatabasehelper_h__
#define mozilla_dom_indexeddb_opendatabasehelper_h__

#include "AsyncConnectionHelper.h"
#include "DatabaseInfo.h"
#include "IDBDatabase.h"
#include "IDBRequest.h"

#include "nsIRunnable.h"

class mozIStorageConnection;

BEGIN_INDEXEDDB_NAMESPACE

class OpenDatabaseHelper : public HelperBase
{
public:
  OpenDatabaseHelper(IDBOpenDBRequest* aRequest,
                     const nsAString& aName,
                     const nsACString& aASCIIOrigin,
                     PRUint64 aRequestedVersion,
                     bool aForDeletion)
    : HelperBase(aRequest), mOpenDBRequest(aRequest), mName(aName),
      mASCIIOrigin(aASCIIOrigin), mRequestedVersion(aRequestedVersion),
      mForDeletion(aForDeletion), mDatabaseId(nullptr), mCurrentVersion(0),
      mLastObjectStoreId(0), mLastIndexId(0), mState(eCreated),
      mResultCode(NS_OK), mLoadDBMetadata(false)
  {
    NS_ASSERTION(!aForDeletion || !aRequestedVersion,
                 "Can't be for deletion and request a version!");
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init();

  nsresult Dispatch(nsIEventTarget* aDatabaseThread);
  nsresult RunImmediately();

  void SetError(nsresult rv)
  {
    NS_ASSERTION(NS_FAILED(rv), "Why are you telling me?");
    mResultCode = rv;
  }

  virtual ChildProcessSendResult
  MaybeSendResponseToChildProcess(nsresult aResultCode) MOZ_OVERRIDE
  {
    return Success_NotSent;
  }

  virtual nsresult GetResultCode() MOZ_OVERRIDE
  {
    return mResultCode;
  }

  nsresult NotifySetVersionFinished();
  nsresult NotifyDeleteFinished();
  void BlockDatabase();

  nsIAtom* Id() const
  {
    return mDatabaseId.get();
  }

  IDBDatabase* Database() const
  {
    NS_ASSERTION(mDatabase, "Calling at the wrong time!");
    return mDatabase;
  }

  static
  nsresult CreateDatabaseConnection(const nsAString& aName,
                                    nsIFile* aDBFile,
                                    nsIFile* aFileManagerDirectory,
                                    mozIStorageConnection** aConnection);

protected:
  
  nsresult EnsureSuccessResult();
  nsresult StartSetVersion();
  nsresult StartDelete();
  virtual nsresult GetSuccessResult(JSContext* aCx,
                                    jsval* aVal) MOZ_OVERRIDE;
  void DispatchSuccessEvent();
  void DispatchErrorEvent();
  virtual void ReleaseMainThreadObjects() MOZ_OVERRIDE;

  
  nsresult DoDatabaseWork();

  
  nsRefPtr<IDBOpenDBRequest> mOpenDBRequest;
  nsString mName;
  nsCString mASCIIOrigin;
  PRUint64 mRequestedVersion;
  bool mForDeletion;
  nsCOMPtr<nsIAtom> mDatabaseId;

  
  nsTArray<nsRefPtr<ObjectStoreInfo> > mObjectStores;
  PRUint64 mCurrentVersion;
  nsString mDatabaseFilePath;
  PRInt64 mLastObjectStoreId;
  PRInt64 mLastIndexId;
  nsRefPtr<IDBDatabase> mDatabase;

  
  enum OpenDatabaseState {
    eCreated = 0, 
    eDBWork, 
    eFiringEvents, 
    eSetVersionPending, 
    eSetVersionCompleted, 
    eDeletePending, 
    eDeleteCompleted, 
  };
  OpenDatabaseState mState;
  nsresult mResultCode;

  nsRefPtr<FileManager> mFileManager;

  nsRefPtr<DatabaseInfo> mDBInfo;
  bool mLoadDBMetadata;
};

END_INDEXEDDB_NAMESPACE

#endif 
