







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history services\n");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
}
catch(ex) {
  do_throw("Could not get the nav-bookmarks-service\n");
}


try {
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}



function run_test() {
  var uri1 = uri("http://foo.bar/");

  
  var bookmark1id = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri1,
                                         bmsvc.DEFAULT_INDEX, "title 1");
  var bookmark2id = bmsvc.insertBookmark(bmsvc.toolbarFolder, uri1,
                                         bmsvc.DEFAULT_INDEX, "title 2");
  
  tagssvc.tagURI(uri1, ["foo", "bar", "foobar", "foo bar"]);

  
  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;

  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, 2);
  var node1 = root.getChild(0);
  do_check_eq(bmsvc.getFolderIdForItem(node1.itemId), bmsvc.bookmarksMenuFolder);
  var node2 = root.getChild(1);
  do_check_eq(bmsvc.getFolderIdForItem(node2.itemId), bmsvc.toolbarFolder);
  root.containerOpen = false;
}
