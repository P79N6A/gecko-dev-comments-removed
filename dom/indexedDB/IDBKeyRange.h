






































#ifndef mozilla_dom_indexeddb_idbkeyrange_h__
#define mozilla_dom_indexeddb_idbkeyrange_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBKeyRange.h"
#include "nsIVariant.h"

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDatabaseRequest;

class IDBKeyRange : public nsIIDBKeyRange
{
  friend class IndexedDatabaseRequest;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBKEYRANGE

protected:
  static
  already_AddRefed<IDBKeyRange> Create(nsIVariant* aLeft,
                                       nsIVariant* aRight,
                                       PRUint16 aFlags);

  IDBKeyRange()
  : mFlags(nsIIDBKeyRange::SINGLE)
  { }

  ~IDBKeyRange() { }

  nsCOMPtr<nsIVariant> mLeft;
  nsCOMPtr<nsIVariant> mRight;
  PRUint16 mFlags;
};

END_INDEXEDDB_NAMESPACE

#endif 
