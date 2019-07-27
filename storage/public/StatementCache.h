





#ifndef mozilla_storage_StatementCache_h
#define mozilla_storage_StatementCache_h

#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozIStorageAsyncStatement.h"

#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

namespace mozilla {
namespace storage {





template<typename StatementType>
class StatementCache {
public:
  









  explicit StatementCache(nsCOMPtr<mozIStorageConnection>& aConnection)
  : mConnection(aConnection)
  {
  }

  








  inline
  already_AddRefed<StatementType>
  GetCachedStatement(const nsACString& aQuery)
  {
    nsCOMPtr<StatementType> stmt;
    if (!mCachedStatements.Get(aQuery, getter_AddRefs(stmt))) {
      stmt = CreateStatement(aQuery);
      NS_ENSURE_TRUE(stmt, nullptr);

      mCachedStatements.Put(aQuery, stmt);
    }
    return stmt.forget();
  }

  template<int N>
  MOZ_ALWAYS_INLINE already_AddRefed<StatementType>
  GetCachedStatement(const char (&aQuery)[N])
  {
    nsDependentCString query(aQuery, N - 1);
    return GetCachedStatement(query);
  }

  



  inline
  void
  FinalizeStatements()
  {
    (void)mCachedStatements.Enumerate(FinalizeCachedStatements, nullptr);

    
    (void)mCachedStatements.Clear();
  }

private:
  inline
  already_AddRefed<StatementType>
  CreateStatement(const nsACString& aQuery);
  static
  PLDHashOperator
  FinalizeCachedStatements(const nsACString& aKey,
                           nsCOMPtr<StatementType>& aStatement,
                           void*)
  {
    (void)aStatement->Finalize();
    return PL_DHASH_NEXT;
  }

  nsInterfaceHashtable<nsCStringHashKey, StatementType> mCachedStatements;
  nsCOMPtr<mozIStorageConnection>& mConnection;
};

template< >
inline
already_AddRefed<mozIStorageStatement>
StatementCache<mozIStorageStatement>::CreateStatement(const nsACString& aQuery)
{
  NS_ENSURE_TRUE(mConnection, nullptr);

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mConnection->CreateStatement(aQuery, getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    nsCString error;
    error.AppendLiteral("The statement '");
    error.Append(aQuery);
    error.AppendLiteral("' failed to compile with the error message '");
    nsCString msg;
    (void)mConnection->GetLastErrorString(msg);
    error.Append(msg);
    error.AppendLiteral("'.");
    NS_ERROR(error.get());
  }
  NS_ENSURE_SUCCESS(rv, nullptr);

  return stmt.forget();
}

template< >
inline
already_AddRefed<mozIStorageAsyncStatement>
StatementCache<mozIStorageAsyncStatement>::CreateStatement(const nsACString& aQuery)
{
  NS_ENSURE_TRUE(mConnection, nullptr);

  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  nsresult rv = mConnection->CreateAsyncStatement(aQuery, getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, nullptr);

  return stmt.forget();
}

} 
} 

#endif 
