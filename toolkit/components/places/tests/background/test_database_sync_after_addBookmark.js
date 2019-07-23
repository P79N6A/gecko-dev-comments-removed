






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";

function run_test()
{
  
  let bh = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  let id = bh.insertBookmark(bh.unfiledBookmarksFolder, uri(TEST_URI),
                             bh.DEFAULT_INDEX, "test");

  PlacesBackground.dispatch(new_test_bookmark_uri_event(id, TEST_URI, true, true),
                            Ci.nsIEventTarget.DISPATCH_NORMAL);
  do_test_pending();
}
