





#ifndef mozilla_dom_indexeddb_idbindex_h__
#define mozilla_dom_indexeddb_idbindex_h__

#include "js/RootingAPI.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/IDBCursorBinding.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupports.h"
#include "nsTArrayForwardDeclare.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

template <typename> class Sequence;

namespace indexedDB {

class IDBObjectStore;
class IDBRequest;
class IndexMetadata;
class Key;
class KeyPath;

class IDBIndex MOZ_FINAL
  : public nsISupports
  , public nsWrapperCache
{
  nsRefPtr<IDBObjectStore> mObjectStore;

  JS::Heap<JS::Value> mCachedKeyPath;

  
  
  
  
  const IndexMetadata* mMetadata;
  nsAutoPtr<IndexMetadata> mDeletedMetadata;

  const int64_t mId;
  bool mRooted;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBIndex)

  static already_AddRefed<IDBIndex>
  Create(IDBObjectStore* aObjectStore, const IndexMetadata& aMetadata);

  int64_t
  Id() const
  {
    AssertIsOnOwningThread();

    return mId;
  }

  const nsString&
  Name() const;

  bool
  Unique() const;

  bool
  MultiEntry() const;

  const KeyPath&
  GetKeyPath() const;

  IDBObjectStore*
  ObjectStore() const
  {
    AssertIsOnOwningThread();
    return mObjectStore;
  }

  nsPIDOMWindow*
  GetParentObject() const;

  void
  GetName(nsString& aName) const
  {
    aName = Name();
  }

  void
  GetKeyPath(JSContext* aCx,
             JS::MutableHandle<JS::Value> aResult,
             ErrorResult& aRv);

  already_AddRefed<IDBRequest>
  OpenCursor(JSContext* aCx,
             JS::Handle<JS::Value> aRange,
             IDBCursorDirection aDirection,
             ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return OpenCursorInternal( false, aCx, aRange, aDirection,
                              aRv);
  }

  already_AddRefed<IDBRequest>
  OpenKeyCursor(JSContext* aCx,
                JS::Handle<JS::Value> aRange,
                IDBCursorDirection aDirection,
                ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return OpenCursorInternal( true, aCx, aRange, aDirection,
                              aRv);
  }

  already_AddRefed<IDBRequest>
  Get(JSContext* aCx, JS::Handle<JS::Value> aKey, ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return GetInternal( false, aCx, aKey, aRv);
  }

  already_AddRefed<IDBRequest>
  GetKey(JSContext* aCx, JS::Handle<JS::Value> aKey, ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return GetInternal( true, aCx, aKey, aRv);
  }

  already_AddRefed<IDBRequest>
  Count(JSContext* aCx, JS::Handle<JS::Value> aKey,
         ErrorResult& aRv);

  already_AddRefed<IDBRequest>
  GetAll(JSContext* aCx, JS::Handle<JS::Value> aKey,
         const Optional<uint32_t>& aLimit, ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return GetAllInternal( false, aCx, aKey, aLimit, aRv);
  }

  already_AddRefed<IDBRequest>
  GetAllKeys(JSContext* aCx, JS::Handle<JS::Value> aKey,
             const Optional<uint32_t>& aLimit, ErrorResult& aRv)
  {
    AssertIsOnOwningThread();

    return GetAllInternal( true, aCx, aKey, aLimit, aRv);
  }

  void
  RefreshMetadata(bool aMayDelete);

  void
  NoteDeletion();

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif


  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  IDBIndex(IDBObjectStore* aObjectStore, const IndexMetadata* aMetadata);

  ~IDBIndex();

  already_AddRefed<IDBRequest>
  GetInternal(bool aKeyOnly,
              JSContext* aCx,
              JS::Handle<JS::Value> aKey,
              ErrorResult& aRv);

  already_AddRefed<IDBRequest>
  GetAllInternal(bool aKeysOnly,
                 JSContext* aCx,
                 JS::Handle<JS::Value> aKey,
                 const Optional<uint32_t>& aLimit,
                 ErrorResult& aRv);

  already_AddRefed<IDBRequest>
  OpenCursorInternal(bool aKeysOnly,
                     JSContext* aCx,
                     JS::Handle<JS::Value> aRange,
                     IDBCursorDirection aDirection,
                     ErrorResult& aRv);
};

} 
} 
} 

#endif 
