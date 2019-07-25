






































#include "TestHarness.h"
#include "nsMemory.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsDocShellCID.h"

#include "nsToolkitCompsCID.h"
#include "nsINavHistoryService.h"
#include "nsIObserverService.h"
#include "mozilla/IHistory.h"

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

#define do_check_eq(aFirst, aSecond) \
  do_check_true(aFirst == aSecond)

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

already_AddRefed<IHistory>
do_get_IHistory()
{
  nsCOMPtr<IHistory> history = do_GetService(NS_IHISTORY_CONTRACTID);
  do_check_true(history);
  return history.forget();
}
