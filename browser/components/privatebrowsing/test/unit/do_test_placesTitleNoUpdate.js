






































Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");

function do_test()
{
  let pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);

  const TEST_URI = uri("http://mozilla.com/privatebrowsing");
  const TITLE_1 = "Title 1";
  const TITLE_2 = "Title 2";

  do_test_pending();
  waitForClearHistory(function() {
    PlacesUtils.bhistory.addPageWithDetails(TEST_URI, TITLE_1, Date.now() * 1000);
    do_check_eq(PlacesUtils.history.getPageTitle(TEST_URI), TITLE_1);

    pb.privateBrowsingEnabled = true;

    PlacesUtils.bhistory.addPageWithDetails(TEST_URI, TITLE_2, Date.now() * 2000);
    do_check_eq(PlacesUtils.history.getPageTitle(TEST_URI), TITLE_1);

    pb.privateBrowsingEnabled = false;

    do_check_eq(PlacesUtils.history.getPageTitle(TEST_URI), TITLE_1);

    pb.privateBrowsingEnabled = true;

    PlacesUtils.bhistory.setPageTitle(TEST_URI, TITLE_2);
    do_check_eq(PlacesUtils.history.getPageTitle(TEST_URI), TITLE_1);

    pb.privateBrowsingEnabled = false;

    do_check_eq(PlacesUtils.history.getPageTitle(TEST_URI), TITLE_1);

    waitForClearHistory(do_test_finished);
  });
}


function run_test() {
  run_test_on_all_services();
}

function waitForClearHistory(aCallback) {
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }
  };
  Services.obs.addObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  PlacesUtils.bhistory.removeAllPages();
}
