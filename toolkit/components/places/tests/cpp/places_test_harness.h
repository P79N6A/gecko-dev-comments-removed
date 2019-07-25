






































#include "TestHarness.h"
#include "nsMemory.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsDocShellCID.h"

#include "nsToolkitCompsCID.h"
#include "nsINavHistoryService.h"
#include "nsIObserverService.h"
#include "mozilla/IHistory.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsPIPlacesDatabase.h"

using namespace mozilla;

static size_t gTotalTests = 0;
static size_t gPassedTests = 0;

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

#ifdef LINUX


#define do_check_eq(aExpected, aActual) \
  do_check_true(aExpected == aActual)
#else
#include <sstream>

#define do_check_eq(aExpected, aActual) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (aExpected == aActual) { \
      gPassedTests++; \
    } else { \
      std::ostringstream temp; \
      temp << "Expected '" << aExpected << "', got '" << aActual <<"' at "; \
      temp << __FILE__ << ":" << __LINE__ << "!"; \
      fail(temp.str().c_str()); \
    } \
  PR_END_MACRO
#endif

struct Test
{
  void (*func)(void);
  const char* const name;
};
#define TEST(aName) \
  {aName, #aName}




void run_next_test();




void do_test_pending();
void do_test_finished();







void
addURI(nsIURI* aURI)
{
  nsCOMPtr<nsINavHistoryService> hist =
    do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);

  PRInt64 id;
  nsresult rv = hist->AddVisit(aURI, PR_Now(), nsnull,
                               nsINavHistoryService::TRANSITION_LINK, PR_FALSE,
                               0, &id);
  do_check_success(rv);
}

struct PlaceRecord
{
  PRInt64 id;
  PRInt32 hidden;
  PRInt32 typed;
  PRInt32 visitCount;
};

struct VisitRecord
{
  PRInt64 id;
  PRInt64 lastVisitId;
  PRInt32 transitionType;
};

already_AddRefed<IHistory>
do_get_IHistory()
{
  nsCOMPtr<IHistory> history = do_GetService(NS_IHISTORY_CONTRACTID);
  do_check_true(history);
  return history.forget();
}

already_AddRefed<nsINavHistoryService>
do_get_NavHistory()
{
  nsCOMPtr<nsINavHistoryService> serv =
    do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);
  do_check_true(serv);
  return serv.forget();
}

already_AddRefed<mozIStorageConnection>
do_get_db()
{
  nsCOMPtr<nsINavHistoryService> history = do_get_NavHistory();
  nsCOMPtr<nsPIPlacesDatabase> database = do_QueryInterface(history);
  do_check_true(database);

  mozIStorageConnection* dbConn;
  nsresult rv = database->GetDBConnection(&dbConn);
  do_check_success(rv);
  return dbConn;
}







void
do_get_place(nsIURI* aURI, PlaceRecord& result)
{
  nsCOMPtr<mozIStorageConnection> dbConn = do_get_db();
  nsCOMPtr<mozIStorageStatement> stmt;

  nsCString spec;
  nsresult rv = aURI->GetSpec(spec);
  do_check_success(rv);

  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, hidden, typed, visit_count FROM moz_places "
    "WHERE url=?1 "
  ), getter_AddRefs(stmt));
  do_check_success(rv);

  rv = stmt->BindUTF8StringByIndex(0, spec);
  do_check_success(rv);

  PRBool hasResults;
  rv = stmt->ExecuteStep(&hasResults);
  do_check_success(rv);
  if (!hasResults) {
    result.id = 0;
    return;
  }

  rv = stmt->GetInt64(0, &result.id);
  do_check_success(rv);
  rv = stmt->GetInt32(1, &result.hidden);
  do_check_success(rv);
  rv = stmt->GetInt32(2, &result.typed);
  do_check_success(rv);
  rv = stmt->GetInt32(3, &result.visitCount);
  do_check_success(rv);
}







void
do_get_lastVisit(PRInt64 placeId, VisitRecord& result)
{
  nsCOMPtr<mozIStorageConnection> dbConn = do_get_db();
  nsCOMPtr<mozIStorageStatement> stmt;

  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, from_visit, visit_type FROM moz_historyvisits "
    "WHERE place_id=?1 "
    "LIMIT 1"
  ), getter_AddRefs(stmt));
  do_check_success(rv);

  rv = stmt->BindInt64ByIndex(0, placeId);
  do_check_success(rv);

  PRBool hasResults;
  rv = stmt->ExecuteStep(&hasResults);
  do_check_success(rv);

  if (!hasResults) {
    result.id = 0;
    return;
  }

  rv = stmt->GetInt64(0, &result.id);
  do_check_success(rv);
  rv = stmt->GetInt64(1, &result.lastVisitId);
  do_check_success(rv);
  rv = stmt->GetInt32(2, &result.transitionType);
  do_check_success(rv);
}
