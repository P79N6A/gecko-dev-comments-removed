



#ifndef mozilla_dom_indexeddb_actorschild_h__
#define mozilla_dom_indexeddb_actorschild_h__

#include "IDBTransaction.h"
#include "js/RootingAPI.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBCursorChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBDatabaseChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBFactoryChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBFactoryRequestChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBRequestChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBSharedTypes.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBTransactionChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBVersionChangeTransactionChild.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsIEventTarget;
struct nsID;
struct PRThread;

namespace mozilla {
namespace ipc {

class BackgroundChildImpl;

} 

namespace dom {
namespace indexedDB {

class FileInfo;
class IDBCursor;
class IDBDatabase;
class IDBFactory;
class IDBMutableFile;
class IDBOpenDBRequest;
class IDBRequest;
class Key;
class PermissionRequestChild;
class PermissionRequestParent;
class SerializedStructuredCloneReadInfo;

class ThreadLocal
{
  friend class nsAutoPtr<ThreadLocal>;
  friend class IDBFactory;

  LoggingInfo mLoggingInfo;
  IDBTransaction* mCurrentTransaction;

#ifdef DEBUG
  PRThread* mOwningThread;
#endif

public:
  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  const LoggingInfo&
  GetLoggingInfo() const
  {
    AssertIsOnOwningThread();

    return mLoggingInfo;
  }

  const nsID&
  Id() const
  {
    AssertIsOnOwningThread();

    return mLoggingInfo.backgroundChildLoggingId();
  }

  int64_t
  NextTransactionSN(IDBTransaction::Mode aMode)
  {
    AssertIsOnOwningThread();
    MOZ_ASSERT(mLoggingInfo.nextTransactionSerialNumber() < INT64_MAX);
    MOZ_ASSERT(mLoggingInfo.nextVersionChangeTransactionSerialNumber() >
                 INT64_MIN);

    if (aMode == IDBTransaction::VERSION_CHANGE) {
      return mLoggingInfo.nextVersionChangeTransactionSerialNumber()--;
    }

    return mLoggingInfo.nextTransactionSerialNumber()++;
  }

  uint64_t
  NextRequestSN()
  {
    AssertIsOnOwningThread();
    MOZ_ASSERT(mLoggingInfo.nextRequestSerialNumber() < UINT64_MAX);

    return mLoggingInfo.nextRequestSerialNumber()++;
  }

  void
  SetCurrentTransaction(IDBTransaction* aCurrentTransaction)
  {
    AssertIsOnOwningThread();

    mCurrentTransaction = aCurrentTransaction;
  }

  IDBTransaction*
  GetCurrentTransaction() const
  {
    AssertIsOnOwningThread();

    return mCurrentTransaction;
  }

private:
  explicit ThreadLocal(const nsID& aBackgroundChildLoggingId);
  ~ThreadLocal();

  ThreadLocal() = delete;
  ThreadLocal(const ThreadLocal& aOther) = delete;
};

class BackgroundFactoryChild final
  : public PBackgroundIDBFactoryChild
{
  friend class mozilla::ipc::BackgroundChildImpl;
  friend class IDBFactory;

  IDBFactory* mFactory;

#ifdef DEBUG
  nsCOMPtr<nsIEventTarget> mOwningThread;
#endif

public:
  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  IDBFactory*
  GetDOMObject() const
  {
    AssertIsOnOwningThread();
    return mFactory;
  }

private:
  
  explicit BackgroundFactoryChild(IDBFactory* aFactory);

  
  ~BackgroundFactoryChild();

  void
  SendDeleteMeInternal();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBackgroundIDBFactoryRequestChild*
  AllocPBackgroundIDBFactoryRequestChild(const FactoryRequestParams& aParams)
                                         override;

  virtual bool
  DeallocPBackgroundIDBFactoryRequestChild(
                                      PBackgroundIDBFactoryRequestChild* aActor)
                                      override;

  virtual PBackgroundIDBDatabaseChild*
  AllocPBackgroundIDBDatabaseChild(const DatabaseSpec& aSpec,
                                   PBackgroundIDBFactoryRequestChild* aRequest)
                                   override;

  virtual bool
  DeallocPBackgroundIDBDatabaseChild(PBackgroundIDBDatabaseChild* aActor)
                                     override;

  bool
  SendDeleteMe() = delete;
};

class BackgroundDatabaseChild;

class BackgroundRequestChildBase
{
protected:
  nsRefPtr<IDBRequest> mRequest;

private:
  bool mActorDestroyed;

public:
  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  IDBRequest*
  GetDOMObject() const
  {
    AssertIsOnOwningThread();
    return mRequest;
  }

  bool
  IsActorDestroyed() const
  {
    AssertIsOnOwningThread();
    return mActorDestroyed;
  }

protected:
  explicit BackgroundRequestChildBase(IDBRequest* aRequest);

  virtual
  ~BackgroundRequestChildBase();

  void
  NoteActorDestroyed();
};

class BackgroundFactoryRequestChild final
  : public BackgroundRequestChildBase
  , public PBackgroundIDBFactoryRequestChild
{
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  friend class IDBFactory;
  friend class BackgroundFactoryChild;
  friend class BackgroundDatabaseChild;
  friend class PermissionRequestChild;
  friend class PermissionRequestParent;

  nsRefPtr<IDBFactory> mFactory;
  const uint64_t mRequestedVersion;
  const bool mIsDeleteOp;

public:
  IDBOpenDBRequest*
  GetOpenDBRequest() const;

private:
  
  BackgroundFactoryRequestChild(IDBFactory* aFactory,
                                IDBOpenDBRequest* aOpenRequest,
                                bool aIsDeleteOp,
                                uint64_t aRequestedVersion);

  
  ~BackgroundFactoryRequestChild();

  bool
  HandleResponse(nsresult aResponse);

  bool
  HandleResponse(const OpenDatabaseRequestResponse& aResponse);

  bool
  HandleResponse(const DeleteDatabaseRequestResponse& aResponse);

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  Recv__delete__(const FactoryRequestResponse& aResponse) override;

  virtual bool
  RecvPermissionChallenge(const PrincipalInfo& aPrincipalInfo) override;

  virtual bool
  RecvBlocked(const uint64_t& aCurrentVersion) override;
};

class BackgroundDatabaseChild final
  : public PBackgroundIDBDatabaseChild
{
  friend class BackgroundFactoryChild;
  friend class BackgroundFactoryRequestChild;
  friend class IDBDatabase;

  nsAutoPtr<DatabaseSpec> mSpec;
  nsRefPtr<IDBDatabase> mTemporaryStrongDatabase;
  BackgroundFactoryRequestChild* mOpenRequestActor;
  IDBDatabase* mDatabase;

public:
  void
  AssertIsOnOwningThread() const
  {
    static_cast<BackgroundFactoryChild*>(Manager())->AssertIsOnOwningThread();
  }

  const DatabaseSpec*
  Spec() const
  {
    AssertIsOnOwningThread();
    return mSpec;
  }

  IDBDatabase*
  GetDOMObject() const
  {
    AssertIsOnOwningThread();
    return mDatabase;
  }

private:
  
  BackgroundDatabaseChild(const DatabaseSpec& aSpec,
                          BackgroundFactoryRequestChild* aOpenRequest);

  
  ~BackgroundDatabaseChild();

  void
  SendDeleteMeInternal();

  void
  EnsureDOMObject();

  void
  ReleaseDOMObject();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBackgroundIDBDatabaseFileChild*
  AllocPBackgroundIDBDatabaseFileChild(PBlobChild* aBlobChild)
                                       override;

  virtual bool
  DeallocPBackgroundIDBDatabaseFileChild(
                                        PBackgroundIDBDatabaseFileChild* aActor)
                                        override;

  virtual PBackgroundIDBTransactionChild*
  AllocPBackgroundIDBTransactionChild(
                                    const nsTArray<nsString>& aObjectStoreNames,
                                    const Mode& aMode)
                                    override;

  virtual bool
  DeallocPBackgroundIDBTransactionChild(PBackgroundIDBTransactionChild* aActor)
                                        override;

  virtual PBackgroundIDBVersionChangeTransactionChild*
  AllocPBackgroundIDBVersionChangeTransactionChild(
                                              const uint64_t& aCurrentVersion,
                                              const uint64_t& aRequestedVersion,
                                              const int64_t& aNextObjectStoreId,
                                              const int64_t& aNextIndexId)
                                              override;

  virtual bool
  RecvPBackgroundIDBVersionChangeTransactionConstructor(
                            PBackgroundIDBVersionChangeTransactionChild* aActor,
                            const uint64_t& aCurrentVersion,
                            const uint64_t& aRequestedVersion,
                            const int64_t& aNextObjectStoreId,
                            const int64_t& aNextIndexId)
                            override;

  virtual bool
  DeallocPBackgroundIDBVersionChangeTransactionChild(
                            PBackgroundIDBVersionChangeTransactionChild* aActor)
                            override;

  virtual bool
  RecvVersionChange(const uint64_t& aOldVersion,
                    const NullableVersion& aNewVersion)
                    override;

  virtual bool
  RecvInvalidate() override;

  bool
  SendDeleteMe() = delete;
};

class BackgroundVersionChangeTransactionChild;

class BackgroundTransactionBase
{
  friend class BackgroundVersionChangeTransactionChild;

  
  
  
  nsRefPtr<IDBTransaction> mTemporaryStrongTransaction;

protected:
  
  
  IDBTransaction* mTransaction;

public:
#ifdef DEBUG
  virtual void
  AssertIsOnOwningThread() const = 0;
#else
  void
  AssertIsOnOwningThread() const
  { }
#endif

  IDBTransaction*
  GetDOMObject() const
  {
    AssertIsOnOwningThread();
    return mTransaction;
  }

protected:
  BackgroundTransactionBase();
  explicit BackgroundTransactionBase(IDBTransaction* aTransaction);

  virtual
  ~BackgroundTransactionBase();

  void
  NoteActorDestroyed();

  void
  NoteComplete();

private:
  
  void
  SetDOMTransaction(IDBTransaction* aDOMObject);
};

class BackgroundTransactionChild final
  : public BackgroundTransactionBase
  , public PBackgroundIDBTransactionChild
{
  friend class BackgroundDatabaseChild;
  friend class IDBDatabase;

public:
#ifdef DEBUG
  virtual void
  AssertIsOnOwningThread() const override;
#endif

  void
  SendDeleteMeInternal();

private:
  
  explicit BackgroundTransactionChild(IDBTransaction* aTransaction);

  
  ~BackgroundTransactionChild();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  bool
  RecvComplete(const nsresult& aResult) override;

  virtual PBackgroundIDBRequestChild*
  AllocPBackgroundIDBRequestChild(const RequestParams& aParams) override;

  virtual bool
  DeallocPBackgroundIDBRequestChild(PBackgroundIDBRequestChild* aActor)
                                    override;

  virtual PBackgroundIDBCursorChild*
  AllocPBackgroundIDBCursorChild(const OpenCursorParams& aParams) override;

  virtual bool
  DeallocPBackgroundIDBCursorChild(PBackgroundIDBCursorChild* aActor)
                                   override;

  bool
  SendDeleteMe() = delete;
};

class BackgroundVersionChangeTransactionChild final
  : public BackgroundTransactionBase
  , public PBackgroundIDBVersionChangeTransactionChild
{
  friend class BackgroundDatabaseChild;

  IDBOpenDBRequest* mOpenDBRequest;

public:
#ifdef DEBUG
  virtual void
  AssertIsOnOwningThread() const override;
#endif

  void
  SendDeleteMeInternal(bool aFailedConstructor);

private:
  
  explicit BackgroundVersionChangeTransactionChild(IDBOpenDBRequest* aOpenDBRequest);

  
  ~BackgroundVersionChangeTransactionChild();

  
  void
  SetDOMTransaction(IDBTransaction* aDOMObject)
  {
    BackgroundTransactionBase::SetDOMTransaction(aDOMObject);
  }

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  bool
  RecvComplete(const nsresult& aResult) override;

  virtual PBackgroundIDBRequestChild*
  AllocPBackgroundIDBRequestChild(const RequestParams& aParams) override;

  virtual bool
  DeallocPBackgroundIDBRequestChild(PBackgroundIDBRequestChild* aActor)
                                    override;

  virtual PBackgroundIDBCursorChild*
  AllocPBackgroundIDBCursorChild(const OpenCursorParams& aParams) override;

  virtual bool
  DeallocPBackgroundIDBCursorChild(PBackgroundIDBCursorChild* aActor)
                                   override;

  bool
  SendDeleteMe() = delete;
};

class BackgroundRequestChild final
  : public BackgroundRequestChildBase
  , public PBackgroundIDBRequestChild
{
  friend class BackgroundTransactionChild;
  friend class BackgroundVersionChangeTransactionChild;
  friend class IDBTransaction;

  nsRefPtr<IDBTransaction> mTransaction;
  nsTArray<nsRefPtr<FileInfo>> mFileInfos;

public:
  void
  HoldFileInfosUntilComplete(nsTArray<nsRefPtr<FileInfo>>& aFileInfos);

private:
  
  explicit
  BackgroundRequestChild(IDBRequest* aRequest);

  
  
  ~BackgroundRequestChild();

  void
  HandleResponse(nsresult aResponse);

  void
  HandleResponse(const Key& aResponse);

  void
  HandleResponse(const nsTArray<Key>& aResponse);

  void
  HandleResponse(const SerializedStructuredCloneReadInfo& aResponse);

  void
  HandleResponse(const nsTArray<SerializedStructuredCloneReadInfo>& aResponse);

  void
  HandleResponse(JS::Handle<JS::Value> aResponse);

  void
  HandleResponse(uint64_t aResponse);

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  Recv__delete__(const RequestResponse& aResponse) override;
};

class BackgroundCursorChild final
  : public PBackgroundIDBCursorChild
{
  friend class BackgroundTransactionChild;
  friend class BackgroundVersionChangeTransactionChild;

  class DelayedDeleteRunnable;

  IDBRequest* mRequest;
  IDBTransaction* mTransaction;
  IDBObjectStore* mObjectStore;
  IDBIndex* mIndex;
  IDBCursor* mCursor;

  
  nsRefPtr<IDBRequest> mStrongRequest;
  nsRefPtr<IDBCursor> mStrongCursor;

  Direction mDirection;

#ifdef DEBUG
  PRThread* mOwningThread;
#endif

public:
  BackgroundCursorChild(IDBRequest* aRequest,
                        IDBObjectStore* aObjectStore,
                        Direction aDirection);

  BackgroundCursorChild(IDBRequest* aRequest,
                        IDBIndex* aIndex,
                        Direction aDirection);

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  void
  SendContinueInternal(const CursorRequestParams& aParams);

  void
  SendDeleteMeInternal();

  IDBRequest*
  GetRequest() const
  {
    AssertIsOnOwningThread();

    return mRequest;
  }

  IDBObjectStore*
  GetObjectStore() const
  {
    AssertIsOnOwningThread();

    return mObjectStore;
  }

  IDBIndex*
  GetIndex() const
  {
    AssertIsOnOwningThread();

    return mIndex;
  }

  Direction
  GetDirection() const
  {
    AssertIsOnOwningThread();

    return mDirection;
  }

private:
  
  
  ~BackgroundCursorChild();

  void
  HandleResponse(nsresult aResponse);

  void
  HandleResponse(const void_t& aResponse);

  void
  HandleResponse(const ObjectStoreCursorResponse& aResponse);

  void
  HandleResponse(const ObjectStoreKeyCursorResponse& aResponse);

  void
  HandleResponse(const IndexCursorResponse& aResponse);

  void
  HandleResponse(const IndexKeyCursorResponse& aResponse);

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvResponse(const CursorResponse& aResponse) override;

  
  bool
  SendContinue(const CursorRequestParams& aParams) = delete;

  bool
  SendDeleteMe() = delete;
};



void
DispatchMutableFileResult(IDBRequest* aRequest,
                          nsresult aResultCode,
                          IDBMutableFile* aMutableFile);

} 
} 
} 

#endif 
