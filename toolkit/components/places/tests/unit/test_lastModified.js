





































 




function run_test() {
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  var itemId = bs.insertBookmark(bs.bookmarksMenuFolder,
                                 uri("http://www.mozilla.org/"),
                                 bs.DEFAULT_INDEX,
                                 "itemTitle");
  do_check_eq(bs.getItemDateAdded(itemId), bs.getItemLastModified(itemId));

  
  
  bs.setItemLastModified(itemId, Date.now() * 1000);
  do_check_neq(bs.getItemDateAdded(itemId), bs.getItemLastModified(itemId));
  bs.setItemDateAdded(itemId, Date.now() * 1000);
  do_check_eq(bs.getItemDateAdded(itemId), bs.getItemLastModified(itemId));

  bs.removeItem(itemId);
}
