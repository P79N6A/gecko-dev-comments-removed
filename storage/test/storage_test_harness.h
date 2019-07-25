






































#include "TestHarness.h"
#include "nsMemory.h"
#include "nsThreadUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"

static int gTotalTests = 0;
static int gPassedTests = 0;

#define do_check_true(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (aCondition) { \
      gPassedTests++; \
    } else { \
      fail("Expected true, got false at %s:%d!", __FILE__, __LINE__); \
    } \
  PR_END_MACRO

#define do_check_false(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (!aCondition) { \
      gPassedTests++; \
    } else { \
      fail("Expected false, got true at %s:%d!", __FILE__, __LINE__); \
    } \
  PR_END_MACRO

#define do_check_success(aResult) \
  do_check_true(NS_SUCCEEDED(aResult))

#define do_check_eq(aFirst, aSecond) \
  do_check_true(aFirst == aSecond)

already_AddRefed<mozIStorageService>
getService()
{
  nsCOMPtr<mozIStorageService> ss =
    do_GetService("@mozilla.org/storage/service;1");
  do_check_true(ss);
  return ss.forget();
}

already_AddRefed<mozIStorageConnection>
getMemoryDatabase()
{
  nsCOMPtr<mozIStorageService> ss = getService();
  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = ss->OpenSpecialDatabase("memory", getter_AddRefs(conn));
  do_check_success(rv);
  return conn.forget();
}

already_AddRefed<mozIStorageConnection>
getDatabase()
{
  nsCOMPtr<nsIFile> dbFile;
  (void)NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                               getter_AddRefs(dbFile));
  NS_ASSERTION(dbFile, "The directory doesn't exists?!");

  nsresult rv = dbFile->Append(NS_LITERAL_STRING("storage_test_db.sqlite"));
  do_check_success(rv);

  nsCOMPtr<mozIStorageService> ss = getService();
  nsCOMPtr<mozIStorageConnection> conn;
  rv = ss->OpenDatabase(dbFile, getter_AddRefs(conn));
  do_check_success(rv);
  return conn.forget();
}
