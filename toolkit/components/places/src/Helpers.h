





































#ifndef mozilla_places_Helpers_h_
#define mozilla_places_Helpers_h_





#include "mozilla/storage.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace places {




class AsyncStatementCallback : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK
  AsyncStatementCallback() {}

protected:
  virtual ~AsyncStatementCallback() {}
};





#define NS_DECL_ASYNCSTATEMENTCALLBACK \
  NS_IMETHOD HandleResult(mozIStorageResultSet *); \
  NS_IMETHOD HandleCompletion(PRUint16);





#ifdef DEBUG
#define RETURN_IF_STMT(_stmt, _sql)                                            \
  PR_BEGIN_MACRO                                                               \
  if (address_of(_stmt) == address_of(aStmt)) {                                \
    if (!_stmt) {                                                              \
      nsresult rv = mDBConn->CreateStatement(_sql, getter_AddRefs(_stmt));     \
      if (NS_FAILED(rv)) {                                                     \
        nsCAutoString err;                                                     \
        (void)mDBConn->GetLastErrorString(err);                                \
        (void)fprintf(stderr, "$$$ compiling statement failed with '%s'\n",    \
                      err.get());                                              \
      }                                                                        \
      NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && _stmt, nsnull);                       \
    }                                                                          \
    return _stmt.get();                                                        \
  }                                                                            \
  PR_END_MACRO
#else
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
#endif




#define DECLARE_AND_ASSIGN_LAZY_STMT(_localStmt, _globalStmt)                  \
  mozIStorageStatement* _localStmt = GetStatement(_globalStmt);                \
  NS_ENSURE_STATE(_localStmt)

#define DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(_localStmt, _globalStmt)           \
  DECLARE_AND_ASSIGN_LAZY_STMT(_localStmt, _globalStmt);                       \
  mozStorageStatementScoper scoper(_localStmt)







class URIBinder 
{
public:
  
  static nsresult Bind(mozIStorageStatement* statement,
                       PRInt32 index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       PRInt32 index,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       PRInt32 index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       PRInt32 index,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       const nsACString& aName,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       const nsACString& aName,
                       const nsACString& aURLString);
};























nsresult GetReversedHostname(nsIURI* aURI, nsString& aRevHost);




void GetReversedHostname(const nsString& aForward, nsString& aRevHost);









void ReverseString(const nsString& aInput, nsString& aReversed);




template<typename StatementType>
class FinalizeStatementCacheProxy : public nsRunnable
{
public:
  





  FinalizeStatementCacheProxy(
    mozilla::storage::StatementCache<StatementType>& aStatementCache
  )
  : mStatementCache(aStatementCache)
  {
  }

  NS_IMETHOD
  Run()
  {
    mStatementCache.FinalizeStatements();
    return NS_OK;
  }

protected:
  mozilla::storage::StatementCache<StatementType>& mStatementCache;
};

} 
} 

#endif 
