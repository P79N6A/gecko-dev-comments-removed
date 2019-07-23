





































#ifndef _MOZSTORAGESTATEMENTROW_H_
#define _MOZSTORAGESTATEMENTROW_H_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"
#include "mozIStorageStatement.h"
#include "nsString.h"
#include "nsVoidArray.h"

class mozStorageStatementRow : public mozIStorageStatementRow,
                               public nsIXPCScriptable
{
public:
    mozStorageStatementRow(mozIStorageStatement *aStatement,
                           int aNumColumns,
                           const nsStringArray *aColumnNames);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_MOZISTORAGESTATEMENTROW

    
    NS_DECL_NSIXPCSCRIPTABLE
protected:
    sqlite3_stmt* NativeStatement() {
        return mStatement->GetNativeStatementPointer();
    }

    nsCOMPtr<mozIStorageStatement> mStatement;
    int mNumColumns;
    const nsStringArray *mColumnNames;
};

#endif 
