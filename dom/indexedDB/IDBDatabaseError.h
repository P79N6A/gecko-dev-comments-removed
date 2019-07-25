






































#ifndef mozilla_dom_indexeddb_idbdatabaseerror_h__
#define mozilla_dom_indexeddb_idbdatabaseerror_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabaseError.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBErrorEvent;

class IDBDatabaseError : public nsIIDBDatabaseError
{
  friend class IDBErrorEvent;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASEERROR

protected:
  IDBDatabaseError(PRUint16 aCode);

protected:
  PRUint16 mCode;
  nsString mMessage;
};

END_INDEXEDDB_NAMESPACE

#endif 
