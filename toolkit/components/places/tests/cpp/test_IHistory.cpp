






































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







Test gTests[] = {
  TEST(test_unvisted_does_not_notify_part1), 
  TEST(test_visited_notifies),
  TEST(test_unvisted_does_not_notify_part2), 
  TEST(test_same_uri_notifies_both),
  TEST(test_unregistered_visited_does_not_notify), 
  TEST(test_new_visit_notifies_waiting_Link),
  TEST(test_RegisterVisitedCallback_returns_before_notifying),
};

const char* file = __FILE__;
#define TEST_NAME "IHistory"
#define TEST_FILE file
#include "places_test_harness_tail.h"
