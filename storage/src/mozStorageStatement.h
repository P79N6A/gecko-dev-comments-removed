






































#ifndef _mozStorageStatement_h_
#define _mozStorageStatement_h_

#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsTArray.h"

#include "mozIStorageStatement.h"

class nsIXPConnectJSObjectHolder;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {
class StatementJSHelper;
class Connection;

class Statement : public mozIStorageStatement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENT
  NS_DECL_MOZISTORAGEVALUEARRAY

  Statement();

  








  nsresult initialize(Connection *aDBConnection,
                      const nsACString &aSQLStatement);


  


  inline sqlite3_stmt *nativeStatement() { return mDBStatement; }

private:
    ~Statement();

    nsRefPtr<Connection> mDBConnection;
    sqlite3_stmt *mDBStatement;
    PRUint32 mParamCount;
    PRUint32 mResultColumnCount;
    nsTArray<nsCString> mColumnNames;
    bool mExecuting;

    



    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementParamsHolder;
    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementRowHolder;

    friend class StatementJSHelper;
};

} 
} 

#endif 
