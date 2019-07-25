






































#ifndef mozilla_dom_indexeddb_idbkeyrange_h__
#define mozilla_dom_indexeddb_idbkeyrange_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBKeyRange.h"
#include "nsIVariant.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBKeyRange : public nsIIDBKeyRange
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBKEYRANGE

  static JSBool DefineConstructors(JSContext* aCx,
                                   JSObject* aObject);

  static
  already_AddRefed<IDBKeyRange> Create(nsIVariant* aLower,
                                       nsIVariant* aUpper,
                                       bool aLowerOpen,
                                       bool aUpperOpen);

protected:
  IDBKeyRange()
  : mLowerOpen(false), mUpperOpen(false)
  { }

  ~IDBKeyRange() { }

  nsCOMPtr<nsIVariant> mLower;
  nsCOMPtr<nsIVariant> mUpper;
  bool mLowerOpen;
  bool mUpperOpen;
};

END_INDEXEDDB_NAMESPACE

#endif 
