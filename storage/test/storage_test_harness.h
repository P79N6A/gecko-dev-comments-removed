






































#include "TestHarness.h"
#include "nsMemory.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"

static size_t gTotalTests = 0;
static size_t gPassedTests = 0;

#define do_check_true(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (aCondition) \
      gPassedTests++; \
    else \
      fail("Expected true, got false on line %d!", __LINE__); \
  PR_END_MACRO

#define do_check_false(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (!aCondition) \
      gPassedTests++; \
    else \
      fail("Expected false, got true on line %d!", __LINE__); \
  PR_END_MACRO

already_AddRefed<mozIStorageConnection>
getMemoryDatabase()
{
  nsCOMPtr<mozIStorageService> ss =
    do_GetService("@mozilla.org/storage/service;1");
  nsCOMPtr<mozIStorageConnection> conn;
  (void)ss->OpenSpecialDatabase("memory", getter_AddRefs(conn));
  return conn.forget();
}
