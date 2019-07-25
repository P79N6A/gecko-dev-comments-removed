






































#ifndef mozilla_dom_indexeddb_indexeddatabaserequest_h__
#define mozilla_dom_indexeddb_indexeddatabaserequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIndexedDatabaseRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDatabaseRequest : public nsIIndexedDatabaseRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINDEXEDDATABASEREQUEST

  static already_AddRefed<nsIIndexedDatabaseRequest> Create();

protected:
  
  IndexedDatabaseRequest();
};

END_INDEXEDDB_NAMESPACE

#endif 
