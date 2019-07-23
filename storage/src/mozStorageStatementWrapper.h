





































#ifndef _MOZSTORAGESTATEMENTWRAPPER_H_
#define _MOZSTORAGESTATEMENTWRAPPER_H_

#include "mozIStorageStatement.h"
#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

#include "nsVoidArray.h"

#include "sqlite3.h"





class mozStorageStatementWrapper : public mozIStorageStatementWrapper,
                                   public nsIXPCScriptable
{
public:
    mozStorageStatementWrapper();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESTATEMENTWRAPPER
    NS_DECL_NSIXPCSCRIPTABLE

private:
    ~mozStorageStatementWrapper();

protected:
    sqlite3_stmt* NativeStatement() {
        return mStatement->GetNativeStatementPointer();
    }

    
    nsCOMPtr<mozIStorageStatement> mStatement;
    PRUint32 mParamCount;
    PRUint32 mResultColumnCount;
    nsStringArray mColumnNames;

    nsCOMPtr<mozIStorageStatementRow> mStatementRow;
    nsCOMPtr<mozIStorageStatementParams> mStatementParams;
};

#endif 
