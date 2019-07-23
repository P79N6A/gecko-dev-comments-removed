






































#ifndef _mozStorageStatementWrapper_h_
#define _mozStorageStatementWrapper_h_

#include "nsTArray.h"
#include "nsIXPCScriptable.h"

#include "mozStorageStatement.h"
#include "mozIStorageStatementWrapper.h"

namespace mozilla {
namespace storage {

class StatementWrapper : public mozIStorageStatementWrapper
                       , public nsIXPCScriptable
{
public:
    StatementWrapper();

    
    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESTATEMENTWRAPPER
    NS_DECL_NSIXPCSCRIPTABLE

private:
    ~StatementWrapper();

    sqlite3_stmt *nativeStatement() {
      return mStatement->nativeStatement();
    }

    nsRefPtr<Statement> mStatement;
    PRUint32 mParamCount;
    PRUint32 mResultColumnCount;
    nsTArray<nsString> mColumnNames;

    nsCOMPtr<mozIStorageStatementRow> mStatementRow;
    nsCOMPtr<mozIStorageStatementParams> mStatementParams;
};

} 
} 

#endif 
