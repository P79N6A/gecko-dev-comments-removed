






































#include "places_test_harness.h"
#include "nsIBrowserHistory.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

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

class VisitURIObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  VisitURIObserver(int aExpectedVisits = 1) :
    mVisits(0),
    mExpectedVisits(aExpectedVisits)
  {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    do_check_true(observerService);
    (void)observerService->AddObserver(this,
                                       "uri-visit-saved",
                                       PR_FALSE);
  }

  void WaitForNotification()
  {
    while (mVisits < mExpectedVisits) {
      (void)NS_ProcessNextEvent();
    }
  }

  NS_IMETHOD Observe(nsISupports* aSubject,
                     const char* aTopic,
                     const PRUnichar* aData)
  {
    mVisits++;

    if (mVisits == mExpectedVisits) {
      nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
      (void)observerService->RemoveObserver(this, "uri-visit-saved");
    }

    return NS_OK;
  }
private:
  int mVisits;
  int mExpectedVisits;
};
NS_IMPL_ISUPPORTS1(
  VisitURIObserver,
  nsIObserver
)




void
test_set_places_enabled()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  do_check_success(rv);

  rv = prefBranch->SetBoolPref("places.history.enabled", PR_TRUE);
  do_check_success(rv);

  
  run_next_test();
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

      
      do_check_eq(visited, mExpectVisit);

      
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

void
test_visituri_inserts()
{
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());

  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);

  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  PlaceRecord place;
  do_get_place(visitedURI, place);

  do_check_true(place.id > 0);
  do_check_false(place.hidden);
  do_check_false(place.typed);
  do_check_eq(place.visitCount, 1);

  run_next_test();
}

void
test_visituri_updates()
{
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());
  nsCOMPtr<VisitURIObserver> finisher;

  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);
  finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);
  finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  PlaceRecord place;
  do_get_place(visitedURI, place);

  do_check_eq(place.visitCount, 2);

  run_next_test();
}

void
test_visituri_preserves_shown_and_typed()
{
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());

  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);
  
  
  history->VisitURI(visitedURI, lastURI, 0);

  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver(2);
  finisher->WaitForNotification();

  PlaceRecord place;
  do_get_place(visitedURI, place);
  do_check_false(place.hidden);

  run_next_test();
}

void
test_visituri_creates_visit()
{
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());

  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);
  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  PlaceRecord place;
  VisitRecord visit;
  do_get_place(visitedURI, place);
  do_get_lastVisit(place.id, visit);

  do_check_true(visit.id > 0);
  do_check_eq(visit.lastVisitId, 0);
  do_check_eq(visit.transitionType, nsINavHistoryService::TRANSITION_LINK);

  run_next_test();
}

void
test_visituri_transition_typed()
{
  nsCOMPtr<nsINavHistoryService> navHistory = do_get_NavHistory();
  nsCOMPtr<nsIBrowserHistory> browserHistory = do_QueryInterface(navHistory);
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());

  browserHistory->MarkPageAsTyped(visitedURI);
  history->VisitURI(visitedURI, lastURI, mozilla::IHistory::TOP_LEVEL);
  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  PlaceRecord place;
  VisitRecord visit;
  do_get_place(visitedURI, place);
  do_get_lastVisit(place.id, visit);

  do_check_true(visit.transitionType == nsINavHistoryService::TRANSITION_TYPED);

  run_next_test();
}

void
test_visituri_transition_embed()
{
  nsCOMPtr<nsINavHistoryService> navHistory = do_get_NavHistory();
  nsCOMPtr<nsIBrowserHistory> browserHistory = do_QueryInterface(navHistory);
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsCOMPtr<nsIURI> lastURI(new_test_uri());
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());

  history->VisitURI(visitedURI, lastURI, 0);
  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  PlaceRecord place;
  VisitRecord visit;
  do_get_place(visitedURI, place);
  do_get_lastVisit(place.id, visit);

  do_check_eq(place.id, 0);
  do_check_eq(visit.id, 0);

  run_next_test();
}

void
test_new_visit_adds_place_guid()
{
  
  nsCOMPtr<nsIURI> visitedURI(new_test_uri());
  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->VisitURI(visitedURI, NULL,
                                  mozilla::IHistory::TOP_LEVEL);
  do_check_success(rv);
  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  
  PlaceRecord place;
  do_get_place(visitedURI, place);
  do_check_eq(place.visitCount, 1);
  do_check_eq(place.guid.Length(), 12);

  run_next_test();
}




void
test_two_null_links_same_uri()
{
  
  
  
  nsCOMPtr<nsIURI> testURI(new_test_uri());

  nsCOMPtr<IHistory> history(do_get_IHistory());
  nsresult rv = history->RegisterVisitedCallback(testURI, NULL);
  do_check_success(rv);
  rv = history->RegisterVisitedCallback(testURI, NULL);
  do_check_success(rv);

  rv = history->VisitURI(testURI, NULL, mozilla::IHistory::TOP_LEVEL);
  do_check_success(rv);

  nsCOMPtr<VisitURIObserver> finisher = new VisitURIObserver();
  finisher->WaitForNotification();

  run_next_test();
}







Test gTests[] = {
  TEST(test_set_places_enabled), 
  TEST(test_unvisted_does_not_notify_part1), 
  TEST(test_visited_notifies),
  TEST(test_unvisted_does_not_notify_part2), 
  TEST(test_same_uri_notifies_both),
  TEST(test_unregistered_visited_does_not_notify), 
  TEST(test_new_visit_notifies_waiting_Link),
  TEST(test_RegisterVisitedCallback_returns_before_notifying),
  TEST(test_observer_topic_dispatched),
  TEST(test_visituri_inserts),
  TEST(test_visituri_updates),
  TEST(test_visituri_preserves_shown_and_typed),
  TEST(test_visituri_creates_visit),
  TEST(test_visituri_transition_typed),
  TEST(test_visituri_transition_embed),
  TEST(test_new_visit_adds_place_guid),

  
  TEST(test_two_null_links_same_uri),
};

const char* file = __FILE__;
#define TEST_NAME "IHistory"
#define TEST_FILE file
#include "places_test_harness_tail.h"
