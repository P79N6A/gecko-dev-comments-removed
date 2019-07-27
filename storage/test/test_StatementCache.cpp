





#include "storage_test_harness.h"

#include "mozilla/Attributes.h"
#include "mozilla/storage/StatementCache.h"
using namespace mozilla::storage;








class SyncCache : public StatementCache<mozIStorageStatement>
{
public:
  explicit SyncCache(nsCOMPtr<mozIStorageConnection>& aConnection)
  : StatementCache<mozIStorageStatement>(aConnection)
  {
  }
};

class AsyncCache : public StatementCache<mozIStorageAsyncStatement>
{
public:
  explicit AsyncCache(nsCOMPtr<mozIStorageConnection>& aConnection)
  : StatementCache<mozIStorageAsyncStatement>(aConnection)
  {
  }
};





class StringWrapper : public nsCString
{
public:
  MOZ_IMPLICIT StringWrapper(const char* aOther)
  {
    this->Assign(aOther);
  }
};




template<typename StringType>
void
test_GetCachedStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());
  SyncCache cache(db);

  StringType sql = "SELECT * FROM sqlite_master";

  
  nsCOMPtr<mozIStorageStatement> stmt = cache.GetCachedStatement(sql);
  do_check_true(stmt);
  int32_t state;
  do_check_success(stmt->GetState(&state));
  do_check_eq(mozIStorageBaseStatement::MOZ_STORAGE_STATEMENT_READY, state);

  
  nsCOMPtr<mozIStorageStatement> stmt2 = cache.GetCachedStatement(sql);
  do_check_true(stmt2);
  do_check_eq(stmt.get(), stmt2.get());
}

template <typename StringType>
void
test_FinalizeStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());
  SyncCache cache(db);

  StringType sql = "SELECT * FROM sqlite_master";

  
  nsCOMPtr<mozIStorageStatement> stmt = cache.GetCachedStatement(sql);
  do_check_true(stmt);

  cache.FinalizeStatements();

  
  int32_t state;
  do_check_success(stmt->GetState(&state));
  do_check_eq(mozIStorageBaseStatement::MOZ_STORAGE_STATEMENT_INVALID, state);

  
  do_check_success(db->Close());
}

template<typename StringType>
void
test_GetCachedAsyncStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());
  AsyncCache cache(db);

  StringType sql = "SELECT * FROM sqlite_master";

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt = cache.GetCachedStatement(sql);
  do_check_true(stmt);
  int32_t state;
  do_check_success(stmt->GetState(&state));
  do_check_eq(mozIStorageBaseStatement::MOZ_STORAGE_STATEMENT_READY, state);

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt2 = cache.GetCachedStatement(sql);
  do_check_true(stmt2);
  do_check_eq(stmt.get(), stmt2.get());
}

template <typename StringType>
void
test_FinalizeAsyncStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());
  AsyncCache cache(db);

  StringType sql = "SELECT * FROM sqlite_master";

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt = cache.GetCachedStatement(sql);
  do_check_true(stmt);

  cache.FinalizeStatements();

  
  int32_t state;
  do_check_success(stmt->GetState(&state));
  do_check_eq(mozIStorageBaseStatement::MOZ_STORAGE_STATEMENT_INVALID, state);

  
  do_check_success(db->AsyncClose(nullptr));
}




void (*gTests[])(void) = {
  test_GetCachedStatement<const char []>,
  test_GetCachedStatement<StringWrapper>,
  test_FinalizeStatements<const char []>,
  test_FinalizeStatements<StringWrapper>,
  test_GetCachedAsyncStatement<const char []>,
  test_GetCachedAsyncStatement<StringWrapper>,
  test_FinalizeAsyncStatements<const char []>,
  test_FinalizeAsyncStatements<StringWrapper>,
};

const char *file = __FILE__;
#define TEST_NAME "StatementCache"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
