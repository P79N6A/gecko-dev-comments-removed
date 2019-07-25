






































#ifndef mozilla_dom_indexeddb_asyncdatabaseconnection_h__
#define mozilla_dom_indexeddb_asyncdatabaseconnection_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

class nsIDOMEventTarget;

BEGIN_INDEXEDDB_NAMESPACE





class AsyncDatabaseConnection
{
public:
  static AsyncDatabaseConnection*
  OpenConnection(const nsAString& aName,
                 const nsAString& aDescription,
                 PRBool aReadOnly);
  ~AsyncDatabaseConnection();

  nsresult
  CreateObjectStore(const nsAString& aName,
                    const nsAString& aKeyPath,
                    PRBool aAutoIncrement,
                    nsIDOMEventTarget* aTarget);

  nsresult
  OpenObjectStore(const nsAString& aName,
                  PRUint16 aMode,
                  nsIDOMEventTarget* aTarget);

private:
  AsyncDatabaseConnection();

  nsString mName;
  nsString mDescription;
  PRBool mReadOnly;
};

END_INDEXEDDB_NAMESPACE

#endif 
