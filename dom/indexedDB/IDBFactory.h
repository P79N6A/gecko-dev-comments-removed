





#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozIStorageConnection.h"

#include "mozilla/dom/BindingUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

class nsIAtom;
class nsIFile;
class nsIFileURL;
class nsIIDBOpenDBRequest;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
class ContentParent;
}
}

BEGIN_INDEXEDDB_NAMESPACE

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
  typedef nsTArray<nsRefPtr<ObjectStoreInfo> > ObjectStoreInfoArray;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBFactory)

  
  static nsresult Create(nsPIDOMWindow* aWindow,
                         const nsACString& aASCIIOrigin,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory);

  
  static nsresult Create(nsPIDOMWindow* aWindow,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory)
  {
    return Create(aWindow, EmptyCString(), aContentParent, aFactory);
  }

  
  
  static nsresult Create(JSContext* aCx,
                         JSObject* aOwningObject,
                         ContentParent* aContentParent,
                         IDBFactory** aFactory);

  
  
  static nsresult Create(ContentParent* aContentParent,
                         IDBFactory** aFactory);

  static already_AddRefed<nsIFileURL>
  GetDatabaseFileURL(nsIFile* aDatabaseFile, const nsACString& aOrigin);

  static already_AddRefed<mozIStorageConnection>
  GetConnection(const nsAString& aDatabaseFilePath,
                const nsACString& aOrigin);

  static nsresult
  SetDefaultPragmas(mozIStorageConnection* aConnection);

  static nsresult
  LoadDatabaseInformation(mozIStorageConnection* aConnection,
                          nsIAtom* aDatabaseId,
                          uint64_t* aVersion,
                          ObjectStoreInfoArray& aObjectStores);

  static nsresult
  SetDatabaseMetadata(DatabaseInfo* aDatabaseInfo,
                      uint64_t aVersion,
                      ObjectStoreInfoArray& aObjectStores);

  nsresult
  OpenInternal(const nsAString& aName,
               int64_t aVersion,
               const nsACString& aASCIIOrigin,
               bool aDeleting,
               JSContext* aCallingCx,
               IDBOpenDBRequest** _retval);

  nsresult
  OpenInternal(const nsAString& aName,
               int64_t aVersion,
               bool aDeleting,
               JSContext* aCallingCx,
               IDBOpenDBRequest** _retval)
  {
    return OpenInternal(aName, aVersion, mASCIIOrigin, aDeleting, aCallingCx,
                        _retval);
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

  
  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  already_AddRefed<nsIIDBOpenDBRequest>
  Open(JSContext* aCx, const NonNull<nsAString>& aName,
       const Optional<uint64_t>& aVersion, ErrorResult& aRv)
  {
    return Open(aCx, nullptr, aName, aVersion, false, aRv);
  }

  already_AddRefed<nsIIDBOpenDBRequest>
  DeleteDatabase(JSContext* aCx, const NonNull<nsAString>& aName,
                 ErrorResult& aRv)
  {
    return Open(aCx, nullptr, aName, Optional<uint64_t>(), true, aRv);
  }

  int16_t
  Cmp(JSContext* aCx, JS::Value aFirst, JS::Value aSecond, ErrorResult& aRv);

  already_AddRefed<nsIIDBOpenDBRequest>
  OpenForPrincipal(JSContext* aCx, nsIPrincipal* aPrincipal,
                   const NonNull<nsAString>& aName,
                   const Optional<uint64_t>& aVersion, ErrorResult& aRv);

  already_AddRefed<nsIIDBOpenDBRequest>
  DeleteForPrincipal(JSContext* aCx, nsIPrincipal* aPrincipal,
                     const NonNull<nsAString>& aName, ErrorResult& aRv);

private:
  IDBFactory();
  ~IDBFactory();

  already_AddRefed<nsIIDBOpenDBRequest>
  Open(JSContext* aCx, nsIPrincipal* aPrincipal, const nsAString& aName,
       const Optional<uint64_t>& aVersion, bool aDelete, ErrorResult& aRv);

  nsCString mASCIIOrigin;

  
  
  nsCOMPtr<nsPIDOMWindow> mWindow;
  JSObject* mOwningObject;

  IndexedDBChild* mActorChild;
  IndexedDBParent* mActorParent;

  mozilla::dom::ContentParent* mContentParent;

  bool mRootedOwningObject;
};

END_INDEXEDDB_NAMESPACE

#endif 
