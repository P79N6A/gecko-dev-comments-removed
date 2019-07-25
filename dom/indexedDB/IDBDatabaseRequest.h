






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabaseRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabaseRequest : public nsIIDBDatabaseRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST

protected:
  IDBDatabaseRequest();
};

END_INDEXEDDB_NAMESPACE

#endif 
