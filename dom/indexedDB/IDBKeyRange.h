





#ifndef mozilla_dom_indexeddb_idbkeyrange_h__
#define mozilla_dom_indexeddb_idbkeyrange_h__

#include "js/RootingAPI.h"
#include "js/Value.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/indexedDB/Key.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupports.h"
#include "nsString.h"

class mozIStorageStatement;
struct PRThread;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;

namespace indexedDB {

class SerializedKeyRange;

class IDBKeyRange MOZ_FINAL
  : public nsISupports
{
  nsCOMPtr<nsISupports> mGlobal;
  Key mLower;
  Key mUpper;
  JS::Heap<JS::Value> mCachedLowerVal;
  JS::Heap<JS::Value> mCachedUpperVal;

  const bool mLowerOpen : 1;
  const bool mUpperOpen : 1;
  const bool mIsOnly : 1;
  bool mHaveCachedLowerVal : 1;
  bool mHaveCachedUpperVal : 1;
  bool mRooted : 1;

#ifdef DEBUG
  PRThread* mOwningThread;
#endif

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBKeyRange)

  static nsresult
  FromJSVal(JSContext* aCx,
            JS::Handle<JS::Value> aVal,
            IDBKeyRange** aKeyRange);

  static already_AddRefed<IDBKeyRange>
  FromSerialized(const SerializedKeyRange& aKeyRange);

  static already_AddRefed<IDBKeyRange>
  Only(const GlobalObject& aGlobal,
       JS::Handle<JS::Value> aValue,
       ErrorResult& aRv);

  static already_AddRefed<IDBKeyRange>
  LowerBound(const GlobalObject& aGlobal,
             JS::Handle<JS::Value> aValue,
             bool aOpen,
             ErrorResult& aRv);

  static already_AddRefed<IDBKeyRange>
  UpperBound(const GlobalObject& aGlobal,
             JS::Handle<JS::Value> aValue,
             bool aOpen,
             ErrorResult& aRv);

  static already_AddRefed<IDBKeyRange>
  Bound(const GlobalObject& aGlobal,
        JS::Handle<JS::Value> aLower,
        JS::Handle<JS::Value> aUpper,
        bool aLowerOpen,
        bool aUpperOpen,
        ErrorResult& aRv);

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

  void
  ToSerialized(SerializedKeyRange& aKeyRange) const;

  const Key&
  Lower() const
  {
    return mLower;
  }

  Key&
  Lower()
  {
    return mLower;
  }

  const Key&
  Upper() const
  {
    return mIsOnly ? mLower : mUpper;
  }

  Key&
  Upper()
  {
    return mIsOnly ? mLower : mUpper;
  }

  bool
  IsOnly() const
  {
    return mIsOnly;
  }

  void
  GetBindingClause(const nsACString& aKeyColumnName,
                   nsACString& _retval) const;

  nsresult
  BindToStatement(mozIStorageStatement* aStatement) const;

  void
  DropJSObjects();

  
  bool
  WrapObject(JSContext* aCx, JS::MutableHandle<JSObject*> aReflector);

  nsISupports*
  GetParentObject() const
  {
    return mGlobal;
  }

  void
  GetLower(JSContext* aCx, JS::MutableHandle<JS::Value> aResult,
           ErrorResult& aRv);

  void
  GetUpper(JSContext* aCx, JS::MutableHandle<JS::Value> aResult,
           ErrorResult& aRv);

  bool
  LowerOpen() const
  {
    return mLowerOpen;
  }

  bool
  UpperOpen() const
  {
    return mUpperOpen;
  }

private:
  IDBKeyRange(nsISupports* aGlobal,
              bool aLowerOpen,
              bool aUpperOpen,
              bool aIsOnly);

  ~IDBKeyRange();
};

} 
} 
} 

#endif 
