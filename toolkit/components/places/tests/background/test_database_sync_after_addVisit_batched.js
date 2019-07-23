






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";

function run_test()
{
  
  let bh = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  let id = null;
  bh.runInBatchMode({
    runBatched: function(aUserData)
    {
      let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
               getService(Ci.nsINavHistoryService);

      id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);
      PlacesBackground.dispatch(new_test_visit_uri_event(id, TEST_URI, false),
                                Ci.nsIEventTarget.DISPATCH_SYNC);
    }
  }, null);
  do_check_neq(id, null);
  PlacesBackground.dispatch(new_test_visit_uri_event(id, TEST_URI, true, true),
                            Ci.nsIEventTarget.DISPATCH_NORMAL);
  do_test_pending();
}
