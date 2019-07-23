






































#ifndef _mozStorageStatementData_h_
#define _mozStorageStatementData_h_

#include "nsAutoPtr.h"
#include "nsTArray.h"

#include "mozStorageBindingParamsArray.h"

struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class StatementData
{
public:
  StatementData(sqlite3_stmt *aStatement,
                already_AddRefed<BindingParamsArray> aParamsArray)
  : mStatement(aStatement)
  , mParamsArray(aParamsArray)
  {
  }
  StatementData(const StatementData &aSource)
  : mStatement(aSource.mStatement)
  , mParamsArray(aSource.mParamsArray)
  {
  }
  StatementData()
  {
  }

  operator sqlite3_stmt *() const
  {
    NS_ASSERTION(mStatement, "NULL sqlite3_stmt being handed off!");
    return mStatement;
  }
  operator BindingParamsArray *() const { return mParamsArray; }

  



  inline void finalize()
  {
    (void)::sqlite3_finalize(mStatement);
    mStatement = NULL;
    mParamsArray = nsnull;
  }

  






  inline bool hasParametersToBeBound() const { return mParamsArray != nsnull; }

private:
  sqlite3_stmt *mStatement;
  nsRefPtr<BindingParamsArray> mParamsArray;
};

} 
} 

#endif 
