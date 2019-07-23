










































const bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

const TEST_URL = "http://www.mozilla.org";

function run_test() {
  
  var itemId = bs.insertBookmark(bs.toolbarFolder,
                                 uri(TEST_URL),
                                 bs.DEFAULT_INDEX,
                                 "");
  
  do_check_eq(bs.getItemTitle(itemId), "");
  
  bs.setItemTitle(itemId, null);
  
  do_check_eq(bs.getItemTitle(itemId), null);
  
  bs.removeItem(itemId);

  
  itemId = bs.insertBookmark(bs.toolbarFolder,
                             uri(TEST_URL),
                             bs.DEFAULT_INDEX,
                             null);
  
  do_check_eq(bs.getItemTitle(itemId), null);
  
  bs.setItemTitle(itemId, "");
  
  do_check_eq(bs.getItemTitle(itemId), "");
  
  bs.removeItem(itemId);
}
