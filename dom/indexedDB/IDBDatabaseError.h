






































#ifndef mozilla_dom_indexeddb_idbdatabaseerror_h__
#define mozilla_dom_indexeddb_idbdatabaseerror_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabaseError.h"

#include "nsStringGlue.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBRequest;

class IDBDatabaseError : public nsIIDBDatabaseError
{
  friend class IDBRequest;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASEERROR

protected:
  
  IDBDatabaseError(PRUint16 aCode,
                   const nsAString& aMessage)
  : mCode(aCode),
    mMessage(aMessage)
  { }

protected:
  PRUint16 mCode;
  nsString mMessage;
};

END_INDEXEDDB_NAMESPACE

#endif 
