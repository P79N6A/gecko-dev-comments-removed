






































#ifndef mozilla_dom_indexeddb_idbkeyrange_h__
#define mozilla_dom_indexeddb_idbkeyrange_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/Key.h"

#include "nsIIDBKeyRange.h"

#include "nsCycleCollectionParticipant.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBKeyRange : public nsIIDBKeyRange
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIIDBKEYRANGE
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBKeyRange)

  static JSBool DefineConstructors(JSContext* aCx,
                                   JSObject* aObject);

  static
  nsresult FromJSVal(JSContext* aCx,
                     const jsval& aVal,
                     IDBKeyRange** aKeyRange);

  IDBKeyRange(bool aLowerOpen, bool aUpperOpen, bool aIsOnly)
  : mCachedLowerVal(JSVAL_VOID), mCachedUpperVal(JSVAL_VOID),
    mLowerOpen(aLowerOpen), mUpperOpen(aUpperOpen), mIsOnly(aIsOnly),
    mHaveCachedLowerVal(false), mHaveCachedUpperVal(false), mRooted(false)
  { }

  const Key& Lower() const
  {
    return mLower;
  }

  Key& Lower()
  {
    return mLower;
  }

  const Key& Upper() const
  {
    return mIsOnly ? mLower : mUpper;
  }

  Key& Upper()
  {
    return mIsOnly ? mLower : mUpper;
  }

  bool IsLowerOpen() const
  {
    return mLowerOpen;
  }

  bool IsUpperOpen() const
  {
    return mUpperOpen;
  }

protected:
  ~IDBKeyRange() { }

  Key mLower;
  Key mUpper;
  jsval mCachedLowerVal;
  jsval mCachedUpperVal;
  bool mLowerOpen;
  bool mUpperOpen;
  bool mIsOnly;
  bool mHaveCachedLowerVal;
  bool mHaveCachedUpperVal;
  bool mRooted;
};

END_INDEXEDDB_NAMESPACE

#endif 
