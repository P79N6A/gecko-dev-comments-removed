





































#ifndef _MOZSTORAGESTATEMENT_H_
#define _MOZSTORAGESTATEMENT_H_

#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsTArray.h"

#include "mozIStorageStatement.h"

#include <sqlite3.h>

class mozStorageConnection;
class nsIXPConnectJSObjectHolder;

namespace mozilla {
namespace storage {
class StatementJSHelper;
}
}

class mozStorageStatement : public mozIStorageStatement
{
public:
    mozStorageStatement();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESTATEMENT
    NS_DECL_MOZISTORAGEVALUEARRAY

    








    nsresult Initialize(mozStorageConnection *aDBConnection,
                        const nsACString &aSQLStatement);


    


    inline sqlite3_stmt *nativeStatement() { return mDBStatement; }

private:
    ~mozStorageStatement();

protected:
    nsRefPtr<mozStorageConnection> mDBConnection;
    sqlite3_stmt *mDBStatement;
    PRUint32 mParamCount;
    PRUint32 mResultColumnCount;
    nsTArray<nsCString> mColumnNames;
    PRBool mExecuting;

    



    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementParamsHolder;
    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementRowHolder;

    friend class mozilla::storage::StatementJSHelper;
};

#endif 
