



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
class PBackgroundIDBFileChild;
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

  ThreadLocal() MOZ_DELETE;
  ThreadLocal(const ThreadLocal& aOther) MOZ_DELETE;
};

class BackgroundFactoryChild MOZ_FINAL
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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundIDBFactoryRequestChild*
  AllocPBackgroundIDBFactoryRequestChild(const FactoryRequestParams& aParams)
                                         MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBFactoryRequestChild(
                                      PBackgroundIDBFactoryRequestChild* aActor)
                                      MOZ_OVERRIDE;

  virtual PBackgroundIDBDatabaseChild*
  AllocPBackgroundIDBDatabaseChild(const DatabaseSpec& aSpec,
                                   PBackgroundIDBFactoryRequestChild* aRequest)
                                   MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBDatabaseChild(PBackgroundIDBDatabaseChild* aActor)
                                     MOZ_OVERRIDE;

  bool
  SendDeleteMe() MOZ_DELETE;
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

class BackgroundFactoryRequestChild MOZ_FINAL
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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const FactoryRequestResponse& aResponse) MOZ_OVERRIDE;

  virtual bool
  RecvPermissionChallenge(const PrincipalInfo& aPrincipalInfo) MOZ_OVERRIDE;

  virtual bool
  RecvBlocked(const uint64_t& aCurrentVersion) MOZ_OVERRIDE;
};

class BackgroundDatabaseChild MOZ_FINAL
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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBackgroundIDBDatabaseFileChild*
  AllocPBackgroundIDBDatabaseFileChild(PBlobChild* aBlobChild)
                                       MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBDatabaseFileChild(
                                        PBackgroundIDBDatabaseFileChild* aActor)
                                        MOZ_OVERRIDE;

  virtual PBackgroundIDBTransactionChild*
  AllocPBackgroundIDBTransactionChild(
                                    const nsTArray<nsString>& aObjectStoreNames,
                                    const Mode& aMode)
                                    MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBTransactionChild(PBackgroundIDBTransactionChild* aActor)
                                        MOZ_OVERRIDE;

  virtual PBackgroundIDBVersionChangeTransactionChild*
  AllocPBackgroundIDBVersionChangeTransactionChild(
                                              const uint64_t& aCurrentVersion,
                                              const uint64_t& aRequestedVersion,
                                              const int64_t& aNextObjectStoreId,
                                              const int64_t& aNextIndexId)
                                              MOZ_OVERRIDE;

  virtual bool
  RecvPBackgroundIDBVersionChangeTransactionConstructor(
                            PBackgroundIDBVersionChangeTransactionChild* aActor,
                            const uint64_t& aCurrentVersion,
                            const uint64_t& aRequestedVersion,
                            const int64_t& aNextObjectStoreId,
                            const int64_t& aNextIndexId)
                            MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBVersionChangeTransactionChild(
                            PBackgroundIDBVersionChangeTransactionChild* aActor)
                            MOZ_OVERRIDE;

  virtual bool
  RecvVersionChange(const uint64_t& aOldVersion,
                    const NullableVersion& aNewVersion)
                    MOZ_OVERRIDE;

  virtual bool
  RecvInvalidate() MOZ_OVERRIDE;

  bool
  SendDeleteMe() MOZ_DELETE;
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

class BackgroundTransactionChild MOZ_FINAL
  : public BackgroundTransactionBase
  , public PBackgroundIDBTransactionChild
{
  friend class BackgroundDatabaseChild;
  friend class IDBDatabase;

public:
#ifdef DEBUG
  virtual void
  AssertIsOnOwningThread() const MOZ_OVERRIDE;
#endif

  void
  SendDeleteMeInternal();

private:
  
  explicit BackgroundTransactionChild(IDBTransaction* aTransaction);

  
  ~BackgroundTransactionChild();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  bool
  RecvComplete(const nsresult& aResult) MOZ_OVERRIDE;

  virtual PBackgroundIDBRequestChild*
  AllocPBackgroundIDBRequestChild(const RequestParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBRequestChild(PBackgroundIDBRequestChild* aActor)
                                    MOZ_OVERRIDE;

  virtual PBackgroundIDBCursorChild*
  AllocPBackgroundIDBCursorChild(const OpenCursorParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBCursorChild(PBackgroundIDBCursorChild* aActor)
                                   MOZ_OVERRIDE;

  bool
  SendDeleteMe() MOZ_DELETE;
};

class BackgroundVersionChangeTransactionChild MOZ_FINAL
  : public BackgroundTransactionBase
  , public PBackgroundIDBVersionChangeTransactionChild
{
  friend class BackgroundDatabaseChild;

  IDBOpenDBRequest* mOpenDBRequest;

public:
#ifdef DEBUG
  virtual void
  AssertIsOnOwningThread() const MOZ_OVERRIDE;
#endif

  void
  SendDeleteMeInternal();

private:
  
  explicit BackgroundVersionChangeTransactionChild(IDBOpenDBRequest* aOpenDBRequest);

  
  ~BackgroundVersionChangeTransactionChild();

  
  void
  SetDOMTransaction(IDBTransaction* aDOMObject)
  {
    BackgroundTransactionBase::SetDOMTransaction(aDOMObject);
  }

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  bool
  RecvComplete(const nsresult& aResult) MOZ_OVERRIDE;

  virtual PBackgroundIDBRequestChild*
  AllocPBackgroundIDBRequestChild(const RequestParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBRequestChild(PBackgroundIDBRequestChild* aActor)
                                    MOZ_OVERRIDE;

  virtual PBackgroundIDBCursorChild*
  AllocPBackgroundIDBCursorChild(const OpenCursorParams& aParams) MOZ_OVERRIDE;

  virtual bool
  DeallocPBackgroundIDBCursorChild(PBackgroundIDBCursorChild* aActor)
                                   MOZ_OVERRIDE;

  bool
  SendDeleteMe() MOZ_DELETE;
};

class BackgroundRequestChild MOZ_FINAL
  : public BackgroundRequestChildBase
  , public PBackgroundIDBRequestChild
{
  friend class BackgroundTransactionChild;
  friend class BackgroundVersionChangeTransactionChild;

  nsRefPtr<IDBTransaction> mTransaction;
  nsTArray<nsRefPtr<FileInfo>> mFileInfos;

public:
  explicit BackgroundRequestChild(IDBRequest* aRequest);

  void
  HoldFileInfosUntilComplete(nsTArray<nsRefPtr<FileInfo>>& aFileInfos);

private:
  
  
  ~BackgroundRequestChild();

  void
  MaybeFinishTransactionEarly();

  bool
  HandleResponse(nsresult aResponse);

  bool
  HandleResponse(const Key& aResponse);

  bool
  HandleResponse(const nsTArray<Key>& aResponse);

  bool
  HandleResponse(const SerializedStructuredCloneReadInfo& aResponse);

  bool
  HandleResponse(const nsTArray<SerializedStructuredCloneReadInfo>& aResponse);

  bool
  HandleResponse(JS::Handle<JS::Value> aResponse);

  bool
  HandleResponse(uint64_t aResponse);

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const RequestResponse& aResponse) MOZ_OVERRIDE;
};

class BackgroundCursorChild MOZ_FINAL
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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvResponse(const CursorResponse& aResponse) MOZ_OVERRIDE;

  
  bool
  SendContinue(const CursorRequestParams& aParams) MOZ_DELETE;

  bool
  SendDeleteMe() MOZ_DELETE;
};



void
DispatchMutableFileResult(IDBRequest* aRequest,
                          nsresult aResultCode,
                          IDBMutableFile* aMutableFile);

} 
} 
} 

#endif 
