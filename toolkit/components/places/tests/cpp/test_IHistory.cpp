






































#include "places_test_harness.h"

#include "mock_Link.h"
using namespace mozilla::dom;








void
expect_visit(nsLinkState aState)
{
  do_check_true(aState == eLinkState_Visited);
}

void
expect_no_visit(nsLinkState aState)
{
  do_check_true(aState == eLinkState_Unvisited);
}

already_AddRefed<nsIURI>
new_test_uri()
{
  
  static PRInt32 specNumber = 0;
  nsCAutoString spec = NS_LITERAL_CSTRING("http://mozilla.org/");
  spec.AppendInt(specNumber++);

  
  nsCOMPtr<nsIURI> testURI;
  nsresult rv = NS_NewURI(getter_AddRefs(testURI), spec);
  do_check_success(rv);
  return testURI.forget();
}






namespace test_unvisited_does_not_notify {
  nsCOMPtr<nsIURI> testURI;
  nsCOMPtr<Link> testLink;
}
void
test_unvisted_does_not_notify_part1()
{
  using namespace test_unvisited_does_not_notify;

  
  
  
  
  

  
  testURI = new_test_uri();

  
  testLink = new mock_Link(expect_no_visit);

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, testLink);
  do_check_success(rv);

  
  run_next_test();
}

void
test_visited_notifies()
{
  
  nsCOMPtr<nsIURI> testURI(new_test_uri());
  addURI(testURI);

  
  
  Link* link = new mock_Link(expect_visit);
  NS_ADDREF(link);

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, link);
  do_check_success(rv);
  
}

void
test_unvisted_does_not_notify_part2()
{
  using namespace test_unvisited_does_not_notify;

  
  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->UnregisterVisitedCallback(testURI, testLink);
  do_check_success(rv);

  
  testURI = nsnull;
  testLink = nsnull;

  
  run_next_test();
}

void
test_same_uri_notifies_both()
{
  
  nsCOMPtr<nsIURI> testURI(new_test_uri());
  addURI(testURI);

  
  
  
  Link* link1 = new mock_Link(expect_visit, false);
  NS_ADDREF(link1);
  Link* link2 = new mock_Link(expect_visit);
  NS_ADDREF(link2);

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, link1);
  do_check_success(rv);
  rv = history->RegisterVisitedCallback(testURI, link2);
  do_check_success(rv);

  
}

void
test_unregistered_visited_does_not_notify()
{
  
  
  

  nsCOMPtr<nsIURI> testURI(new_test_uri());
  nsCOMPtr<Link> link(new mock_Link(expect_no_visit));

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, link);
  do_check_success(rv);

  
  rv = history->UnregisterVisitedCallback(testURI, link);
  do_check_success(rv);

  
  addURI(testURI);

  
  
  
  

  
  run_next_test();
}

void
test_new_visit_notifies_waiting_Link()
{
  
  
  Link* link = new mock_Link(expect_visit);
  NS_ADDREF(link);

  
  nsCOMPtr<nsIURI> testURI(new_test_uri());
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, link);
  do_check_success(rv);

  
  addURI(testURI);

  
}

void
test_RegisterVisitedCallback_returns_before_notifying()
{
  
  nsCOMPtr<nsIURI> testURI(new_test_uri());
  addURI(testURI);

  
  nsCOMPtr<Link> link(new mock_Link(expect_no_visit));

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, link);
  do_check_success(rv);

  
  
  rv = history->UnregisterVisitedCallback(testURI, link);
  do_check_success(rv);

  run_next_test();
}

namespace test_observer_topic_dispatched_helpers {
  #define URI_VISITED "visited"
  #define URI_NOT_VISITED "not visited"
  #define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"
  class statusObserver : public nsIObserver
  {
  public:
    NS_DECL_ISUPPORTS

    statusObserver(nsIURI* aURI,
                   const bool aExpectVisit,
                   bool& _notified)
    : mURI(aURI)
    , mExpectVisit(aExpectVisit)
    , mNotified(_notified)
    {
      nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
      do_check_true(observerService);
      (void)observerService->AddObserver(this,
                                         URI_VISITED_RESOLUTION_TOPIC,
                                         PR_FALSE);
    }

    NS_IMETHOD Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const PRUnichar* aData)
    {
      
      do_check_false(strcmp(aTopic, URI_VISITED_RESOLUTION_TOPIC));

      
      nsCOMPtr<nsIURI> notifiedURI(do_QueryInterface(aSubject));
      do_check_true(notifiedURI);
      PRBool isOurURI;
      nsresult rv = notifiedURI->Equals(mURI, &isOurURI);
      do_check_success(rv);
      if (!isOurURI) {
        return NS_OK;
      }

      
      bool visited = !!NS_LITERAL_STRING(URI_VISITED).Equals(aData);
      bool notVisited = !!NS_LITERAL_STRING(URI_NOT_VISITED).Equals(aData);
      do_check_true(visited || notVisited);

      
      do_check_eq(mExpectVisit, visited);

      
      mNotified = true;

      
      nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
      (void)observerService->RemoveObserver(this,
                                            URI_VISITED_RESOLUTION_TOPIC);
      return NS_OK;
    }
  private:
    nsCOMPtr<nsIURI> mURI;
    const bool mExpectVisit;
    bool& mNotified;
  };
  NS_IMPL_ISUPPORTS1(
    statusObserver,
    nsIObserver
  )
}
void
test_observer_topic_dispatched()
{
  using namespace test_observer_topic_dispatched_helpers;

  
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());
  nsCOMPtr<nsIURI> notVisitedURI(new_test_uri());
  PRBool urisEqual;
  nsresult rv = visitedURI->Equals(notVisitedURI, &urisEqual);
  do_check_success(rv);
  do_check_false(urisEqual);
  addURI(visitedURI);

  
  nsCOMPtr<Link> visitedLink(new mock_Link(expect_visit, false));
  NS_ADDREF(visitedLink); 
  nsCOMPtr<Link> notVisitedLink(new mock_Link(expect_no_visit));

  
  bool visitedNotified = false;
  nsCOMPtr<nsIObserver> vistedObs =
    new statusObserver(visitedURI, true, visitedNotified);
  bool notVisitedNotified = false;
  nsCOMPtr<nsIObserver> unvistedObs =
    new statusObserver(notVisitedURI, false, notVisitedNotified);

  
  nsCOMPtr<IHistory> history(do_get_IHistory());
  rv = history->RegisterVisitedCallback(visitedURI, visitedLink);
  do_check_success(rv);
  rv = history->RegisterVisitedCallback(notVisitedURI, notVisitedLink);
  do_check_success(rv);

  
  while (!visitedNotified || !notVisitedNotified) {
    (void)NS_ProcessNextEvent();
  }

  
  rv = history->UnregisterVisitedCallback(notVisitedURI, notVisitedLink);
  do_check_success(rv);

  run_next_test();
}







Test gTests[] = {
  TEST(test_unvisted_does_not_notify_part1), 
  TEST(test_visited_notifies),
  TEST(test_unvisted_does_not_notify_part2), 
  TEST(test_same_uri_notifies_both),
  TEST(test_unregistered_visited_does_not_notify), 
  TEST(test_new_visit_notifies_waiting_Link),
  TEST(test_RegisterVisitedCallback_returns_before_notifying),
  TEST(test_observer_topic_dispatched),
};

const char* file = __FILE__;
#define TEST_NAME "IHistory"
#define TEST_FILE file
#include "places_test_harness_tail.h"
