






































#ifndef _mozStorageStatementData_h_
#define _mozStorageStatementData_h_

#include "sqlite3.h"

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
                already_AddRefed<BindingParamsArray> aParamsArray,
                nsISupports *aStatementOwner)
  : mStatement(aStatement)
  , mParamsArray(aParamsArray)
  , mStatementOwner(aStatementOwner)
  {
  }
  StatementData(const StatementData &aSource)
  : mStatement(aSource.mStatement)
  , mParamsArray(aSource.mParamsArray)
  , mStatementOwner(aSource.mStatementOwner)
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
    (void)::sqlite3_reset(mStatement);
    (void)::sqlite3_clear_bindings(mStatement);
    mStatement = NULL;
    mParamsArray = nsnull;
    mStatementOwner = nsnull;
  }

  






  inline bool hasParametersToBeBound() const { return mParamsArray != nsnull; }
  




  inline bool needsTransaction() const
  {
    return mParamsArray != nsnull && mParamsArray->length() > 1;
  }

private:
  sqlite3_stmt *mStatement;
  nsRefPtr<BindingParamsArray> mParamsArray;

  



  nsCOMPtr<nsISupports> mStatementOwner;
};

} 
} 

#endif 
