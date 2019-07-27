





#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/IDBTransactionBinding.h"
#include "mozilla/dom/StorageTypeBinding.h"
#include "mozilla/dom/indexedDB/IDBWrapperCache.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsString.h"
#include "nsTHashtable.h"

class nsIDocument;
class nsIWeakReference;
class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;
class EventChainPostVisitor;

namespace dom {

class File;
class DOMStringList;
struct IDBObjectStoreParameters;
template <typename> class Sequence;

namespace indexedDB {

class BackgroundDatabaseChild;
class DatabaseSpec;
class FileManager;
class IDBFactory;
class IDBMutableFile;
class IDBObjectStore;
class IDBRequest;
class IDBTransaction;
class PBackgroundIDBDatabaseFileChild;

class IDBDatabase MOZ_FINAL
  : public IDBWrapperCache
{
  typedef mozilla::dom::StorageType StorageType;
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  class Observer;
  friend class Observer;

  
  
  
  nsRefPtr<IDBFactory> mFactory;

  nsAutoPtr<DatabaseSpec> mSpec;

  
  nsAutoPtr<DatabaseSpec> mPreviousSpec;

  nsRefPtr<FileManager> mFileManager;

  BackgroundDatabaseChild* mBackgroundActor;

  nsTHashtable<nsPtrHashKey<IDBTransaction>> mTransactions;

  nsDataHashtable<nsISupportsHashKey, PBackgroundIDBDatabaseFileChild*>
    mFileActors;

  nsTHashtable<nsISupportsHashKey> mReceivedBlobs;

  nsRefPtr<Observer> mObserver;

  
  nsTArray<IDBMutableFile*> mLiveMutableFiles;

  bool mClosed;
  bool mInvalidated;

public:
  static already_AddRefed<IDBDatabase>
  Create(IDBWrapperCache* aOwnerCache,
         IDBFactory* aFactory,
         BackgroundDatabaseChild* aActor,
         DatabaseSpec* aSpec);

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  const nsString&
  Name() const;

  void
  GetName(nsAString& aName) const
  {
    AssertIsOnOwningThread();

    aName = Name();
  }

  uint64_t
  Version() const;

  already_AddRefed<nsIDocument>
  GetOwnerDocument() const;

  void
  Close()
  {
    AssertIsOnOwningThread();

    CloseInternal();
  }

  bool
  IsClosed() const
  {
    AssertIsOnOwningThread();

    return mClosed;
  }

  void
  Invalidate();

  
  
  bool
  IsInvalidated() const
  {
    AssertIsOnOwningThread();

    return mInvalidated;
  }

  void
  EnterSetVersionTransaction(uint64_t aNewVersion);

  void
  ExitSetVersionTransaction();

  
  
  void
  RevertToPreviousState();

  IDBFactory*
  Factory() const
  {
    AssertIsOnOwningThread();

    return mFactory;
  }

  void
  RegisterTransaction(IDBTransaction* aTransaction);

  void
  UnregisterTransaction(IDBTransaction* aTransaction);

  void
  AbortTransactions();

  PBackgroundIDBDatabaseFileChild*
  GetOrCreateFileActorForBlob(File* aBlob);

  void
  NoteFinishedFileActor(PBackgroundIDBDatabaseFileChild* aFileActor);

  void
  NoteReceivedBlob(File* aBlob);

  void
  DelayedMaybeExpireFileActors();

  
  
  nsresult
  GetQuotaInfo(nsACString& aOrigin, PersistenceType* aPersistenceType);

  void
  NoteLiveMutableFile(IDBMutableFile* aMutableFile);

  void
  NoteFinishedMutableFile(IDBMutableFile* aMutableFile);

  nsPIDOMWindow*
  GetParentObject() const;

  already_AddRefed<DOMStringList>
  ObjectStoreNames() const;

  already_AddRefed<IDBObjectStore>
  CreateObjectStore(JSContext* aCx,
                    const nsAString& aName,
                    const IDBObjectStoreParameters& aOptionalParameters,
                    ErrorResult& aRv);

  void
  DeleteObjectStore(const nsAString& name, ErrorResult& aRv);

  already_AddRefed<IDBTransaction>
  Transaction(const nsAString& aStoreName,
              IDBTransactionMode aMode,
              ErrorResult& aRv);

  already_AddRefed<IDBTransaction>
  Transaction(const Sequence<nsString>& aStoreNames,
              IDBTransactionMode aMode,
              ErrorResult& aRv);

  StorageType
  Storage() const;

  IMPL_EVENT_HANDLER(abort)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(versionchange)

  already_AddRefed<IDBRequest>
  CreateMutableFile(const nsAString& aName,
                    const Optional<nsAString>& aType,
                    ErrorResult& aRv);

  already_AddRefed<IDBRequest>
  MozCreateFileHandle(const nsAString& aName,
                      const Optional<nsAString>& aType,
                      ErrorResult& aRv)
  {
    return CreateMutableFile(aName, aType, aRv);
  }

  void
  ClearBackgroundActor()
  {
    AssertIsOnOwningThread();

    mBackgroundActor = nullptr;
  }

  const DatabaseSpec*
  Spec() const
  {
    return mSpec;
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBDatabase, IDBWrapperCache)

  
  virtual void
  LastRelease() MOZ_OVERRIDE;

  virtual nsresult
  PostHandleEvent(EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  IDBDatabase(IDBWrapperCache* aOwnerCache,
              IDBFactory* aFactory,
              BackgroundDatabaseChild* aActor,
              DatabaseSpec* aSpec);

  ~IDBDatabase();

  void
  CloseInternal();

  void
  InvalidateInternal();

  bool
  RunningVersionChangeTransaction() const
  {
    AssertIsOnOwningThread();

    return !!mPreviousSpec;
  }

  void
  RefreshSpec(bool aMayDelete);

  void
  ExpireFileActors(bool aExpireAll);

  void
  InvalidateMutableFiles();
};

} 
} 
} 

#endif 
