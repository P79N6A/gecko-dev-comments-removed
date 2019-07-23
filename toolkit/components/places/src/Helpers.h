





































#ifndef mozilla_places_Helpers_h_
#define mozilla_places_Helpers_h_





#include "mozIStorageStatementCallback.h"

namespace mozilla {
namespace places {




class AsyncStatementCallback : public mozIStorageStatementCallback
{
public:
  
  NS_IMETHOD HandleError(mozIStorageError *aError);
};





#define NS_DECL_ASYNCSTATEMENTCALLBACK \
  NS_IMETHOD HandleResult(mozIStorageResultSet *); \
  NS_IMETHOD HandleCompletion(PRUint16);





#define RETURN_IF_STMT(_stmt, _sql)                                            \
  PR_BEGIN_MACRO                                                               \
  if (address_of(_stmt) == address_of(aStmt)) {                                \
    if (!_stmt) {                                                              \
      nsresult rv = mDBConn->CreateStatement(_sql, getter_AddRefs(_stmt));     \
      NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && _stmt, nsnull);                       \
    }                                                                          \
    return _stmt.get();                                                        \
  }                                                                            \
  PR_END_MACRO




#define DECLARE_AND_ASSIGN_LAZY_STMT(_localStmt, _globalStmt)                  \
  mozIStorageStatement* _localStmt = GetStatement(_globalStmt);                \
  NS_ENSURE_STATE(_localStmt)

#define DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(_localStmt, _globalStmt)           \
  DECLARE_AND_ASSIGN_LAZY_STMT(_localStmt, _globalStmt);                       \
  mozStorageStatementScoper scoper(_localStmt)

} 
} 

#endif 
