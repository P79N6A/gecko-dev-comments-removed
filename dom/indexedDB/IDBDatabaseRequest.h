






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/AsyncDatabaseConnection.h"

#include "nsIIDBDatabaseRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabaseRequest : public IDBRequest::Generator,
                           public nsIIDBDatabaseRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST

  static already_AddRefed<nsIIDBDatabaseRequest>
  Create(const nsAString& aName,
         const nsAString& aDescription,
         PRBool aReadOnly);

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

private:
  nsAutoPtr<AsyncDatabaseConnection> mDatabase;
};

END_INDEXEDDB_NAMESPACE

#endif 
