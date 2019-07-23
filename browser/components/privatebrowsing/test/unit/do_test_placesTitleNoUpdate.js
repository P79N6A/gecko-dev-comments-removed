






































function do_test()
{
  let pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);
  let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  let bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);

  const TEST_URI = uri("http://mozilla.com/privatebrowsing");
  const TITLE_1 = "Title 1";
  const TITLE_2 = "Title 2";

  bhist.removeAllPages();

  bhist.addPageWithDetails(TEST_URI, TITLE_1, Date.now() * 1000);
  do_check_eq(histsvc.getPageTitle(TEST_URI), TITLE_1);

  pb.privateBrowsingEnabled = true;

  bhist.addPageWithDetails(TEST_URI, TITLE_2, Date.now() * 2000);
  do_check_eq(histsvc.getPageTitle(TEST_URI), TITLE_1);

  pb.privateBrowsingEnabled = false;

  do_check_eq(histsvc.getPageTitle(TEST_URI), TITLE_1);

  pb.privateBrowsingEnabled = true;

  bhist.setPageTitle(TEST_URI, TITLE_2);
  do_check_eq(histsvc.getPageTitle(TEST_URI), TITLE_1);

  pb.privateBrowsingEnabled = false;

  do_check_eq(histsvc.getPageTitle(TEST_URI), TITLE_1);

  bhist.removeAllPages();
}


function run_test() {
  run_test_on_all_services();
}
