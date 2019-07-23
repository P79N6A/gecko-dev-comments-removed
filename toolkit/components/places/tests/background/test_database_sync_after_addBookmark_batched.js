






































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
      id = bh.insertBookmark(bh.unfiledBookmarksFolder, uri(TEST_URI),
                             bh.DEFAULT_INDEX, "test");

      PlacesBackground.dispatch(new_test_bookmark_uri_event(id, TEST_URI, false),
                                Ci.nsIEventTarget.DISPATCH_SYNC);
    }
  }, null);
  do_check_neq(id, null);
  PlacesBackground.dispatch(new_test_bookmark_uri_event(id, TEST_URI, true, true),
                            Ci.nsIEventTarget.DISPATCH_NORMAL);
  do_test_pending();
}
