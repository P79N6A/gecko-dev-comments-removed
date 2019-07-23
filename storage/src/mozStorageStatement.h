





































#ifndef _MOZSTORAGESTATEMENT_H_
#define _MOZSTORAGESTATEMENT_H_

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsVoidArray.h"

#include "mozIStorageStatement.h"
#include "mozIStorageConnection.h"

#include <sqlite3.h>

class mozStorageStatement : public mozIStorageStatement
{
public:
    mozStorageStatement();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESTATEMENT
    NS_DECL_MOZISTORAGEVALUEARRAY

private:
    ~mozStorageStatement();

protected:
    nsCString mStatementString;
    nsCOMPtr<mozIStorageConnection> mDBConnection;
    sqlite3_stmt *mDBStatement;
    PRUint32 mParamCount;
    PRUint32 mResultColumnCount;
    nsStringArray mColumnNames;
    PRBool mExecuting;

    
    nsresult Recreate();
};

#endif 
