





#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/BindingDeclarations.h" 
#include "mozilla/dom/StorageTypeBinding.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/dom/quota/StoragePrivilege.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

class mozIStorageConnection;
class nsIFile;
class nsIFileURL;
class nsIPrincipal;
class nsPIDOMWindow;
template<typename> class nsRefPtr;

namespace mozilla {
class ErrorResult;

namespace dom {
class ContentParent;
class IDBOpenDBOptions;

namespace indexedDB {

struct DatabaseInfo;
class IDBDatabase;
class IDBOpenDBRequest;
class IndexedDBChild;
class IndexedDBParent;

struct ObjectStoreInfo;

class IDBFactory MOZ_FINAL : public nsISupports,
                             public nsWrapperCache
{
  typedef mozilla::dom::ContentParent ContentParent;
  typedef mozilla::dom::quota::PersistenceType PersistenceType;
  typedef nsTArray<nsRefPtr<ObjectStoreInfo> > ObjectStoreInfoArray;
  typedef mozilla::dom::quota::StoragePrivilege StoragePrivilege;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBFactory)

  
  static nsresult Create(nsPIDOMWindow* aWindow,
                         const nsACString& aGroup,
                         const nsACString& aASCIIOrigin,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory);

  
  static nsresult Create(nsPIDOMWindow* aWindow,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory)
  {
    return Create(aWindow, EmptyCString(), EmptyCString(), aContentParent,
                  aFactory);
  }

  
  
  static nsresult Create(JSContext* aCx,
                         JS::Handle<JSObject*> aOwningObject,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory);

  
  
  static nsresult Create(ContentParent* aContentParent,
                         IDBFactory** aFactory);

  static already_AddRefed<nsIFileURL>
  GetDatabaseFileURL(nsIFile* aDatabaseFile,
                     PersistenceType aPersistenceType,
                     const nsACString& aGroup,
                     const nsACString& aOrigin);

  static already_AddRefed<mozIStorageConnection>
  GetConnection(const nsAString& aDatabaseFilePath,
                PersistenceType aPersistenceType,
                const nsACString& aGroup,
                const nsACString& aOrigin);

  static nsresult
  SetDefaultPragmas(mozIStorageConnection* aConnection);

  static nsresult
  LoadDatabaseInformation(mozIStorageConnection* aConnection,
                          const nsACString& aDatabaseId,
                          uint64_t* aVersion,
                          ObjectStoreInfoArray& aObjectStores);

  static nsresult
  SetDatabaseMetadata(DatabaseInfo* aDatabaseInfo,
                      uint64_t aVersion,
                      ObjectStoreInfoArray& aObjectStores);

  nsresult
  OpenInternal(const nsAString& aName,
               int64_t aVersion,
               PersistenceType aPersistenceType,
               const nsACString& aGroup,
               const nsACString& aASCIIOrigin,
               StoragePrivilege aStoragePrivilege,
               bool aDeleting,
               IDBOpenDBRequest** _retval);

  nsresult
  OpenInternal(const nsAString& aName,
               int64_t aVersion,
               PersistenceType aPersistenceType,
               bool aDeleting,
               IDBOpenDBRequest** _retval)
  {
    return OpenInternal(aName, aVersion, aPersistenceType, mGroup, mASCIIOrigin,
                        mPrivilege, aDeleting, _retval);
  }

  void
  SetActor(IndexedDBChild* aActorChild)
  {
    NS_ASSERTION(!aActorChild || !mActorChild, "Shouldn't have more than one!");
    mActorChild = aActorChild;
  }

  void
  SetActor(IndexedDBParent* aActorParent)
  {
    NS_ASSERTION(!aActorParent || !mActorParent, "Shouldn't have more than one!");
    mActorParent = aActorParent;
  }

  const nsCString&
  GetASCIIOrigin() const
  {
    return mASCIIOrigin;
  }

  bool
  FromIPC()
  {
    return !!mContentParent;
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  already_AddRefed<IDBOpenDBRequest>
  Open(const nsAString& aName, uint64_t aVersion, ErrorResult& aRv)
  {
    return Open(nullptr, aName, Optional<uint64_t>(aVersion),
                Optional<mozilla::dom::StorageType>(), false, aRv);
  }

  already_AddRefed<IDBOpenDBRequest>
  Open(const nsAString& aName, const IDBOpenDBOptions& aOptions,
       ErrorResult& aRv);

  already_AddRefed<IDBOpenDBRequest>
  DeleteDatabase(const nsAString& aName, const IDBOpenDBOptions& aOptions,
                 ErrorResult& aRv);

  int16_t
  Cmp(JSContext* aCx, JS::Handle<JS::Value> aFirst,
      JS::Handle<JS::Value> aSecond, ErrorResult& aRv);

  already_AddRefed<IDBOpenDBRequest>
  OpenForPrincipal(nsIPrincipal* aPrincipal, const nsAString& aName,
                   uint64_t aVersion, ErrorResult& aRv);

  already_AddRefed<IDBOpenDBRequest>
  OpenForPrincipal(nsIPrincipal* aPrincipal, const nsAString& aName,
                   const IDBOpenDBOptions& aOptions, ErrorResult& aRv);

  already_AddRefed<IDBOpenDBRequest>
  DeleteForPrincipal(nsIPrincipal* aPrincipal, const nsAString& aName,
                     const IDBOpenDBOptions& aOptions, ErrorResult& aRv);

private:
  IDBFactory();
  ~IDBFactory();

  already_AddRefed<IDBOpenDBRequest>
  Open(nsIPrincipal* aPrincipal, const nsAString& aName,
       const Optional<uint64_t>& aVersion,
       const Optional<mozilla::dom::StorageType>& aStorageType, bool aDelete,
       ErrorResult& aRv);

  nsCString mGroup;
  nsCString mASCIIOrigin;
  StoragePrivilege mPrivilege;
  PersistenceType mDefaultPersistenceType;

  
  
  nsCOMPtr<nsPIDOMWindow> mWindow;
  JS::Heap<JSObject*> mOwningObject;

  IndexedDBChild* mActorChild;
  IndexedDBParent* mActorParent;

  mozilla::dom::ContentParent* mContentParent;

  bool mRootedOwningObject;
};

} 
} 
} 

#endif 
