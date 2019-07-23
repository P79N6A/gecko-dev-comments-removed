





































 




function run_test() {
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  var itemId = bs.insertBookmark(bs.bookmarksMenuFolder,
                                 uri("http://www.mozilla.org/"),
                                 bs.DEFAULT_INDEX,
                                 "itemTitle");
  var dateAdded = bs.getItemDateAdded(itemId);
  do_check_eq(dateAdded, bs.getItemLastModified(itemId));

  
  
  
  
  bs.setItemLastModified(itemId, dateAdded + 1);
  do_check_true(bs.getItemLastModified(itemId) === dateAdded + 1);
  do_check_true(bs.getItemDateAdded(itemId) < bs.getItemLastModified(itemId));
  bs.setItemDateAdded(itemId, dateAdded + 2);
  do_check_true(bs.getItemDateAdded(itemId) === dateAdded + 2);
  do_check_eq(bs.getItemDateAdded(itemId), bs.getItemLastModified(itemId));

  bs.removeItem(itemId);
}
