








































try {
  var lmsvc = Cc["@mozilla.org/browser/livemark-service;2"].
              getService(Ci.nsILivemarkService);
} catch(ex) {
  do_throw("Could not get livemark-service\n");
} 


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}


var root = bmsvc.bookmarksMenuFolder;


function run_test() {
  var livemarkId = 
    lmsvc.createLivemarkFolderOnly(root, "foo",
                                   uri("http://example.com/"),
                                   uri("http://example.com/rss.xml"), -1);

  do_check_true(lmsvc.isLivemark(livemarkId));
  do_check_true(lmsvc.getSiteURI(livemarkId).spec == "http://example.com/");
  do_check_true(lmsvc.getFeedURI(livemarkId).spec == "http://example.com/rss.xml");
  var livemarkItem = bmsvc.insertBookmark(livemarkId, uri("http://example.com/item1.html"), bmsvc.DEFAULT_INDEX, "item 1");

  
  var parent = bmsvc.createFolder(root, "test", bmsvc.DEFAULT_INDEX);
  
  var nonLivemarkItem = bmsvc.insertBookmark(parent, uri("http://example.com/item2.html"), bmsvc.DEFAULT_INDEX, "item 2");

  
  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var query = histsvc.getNewQuery();
  query.searchTerms = "item";
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  var cc = rootNode.childCount;
  do_check_eq(cc, 2);
  var node = rootNode.getChild(0);
  do_check_eq(node.itemId, livemarkItem);
  node = rootNode.getChild(1);
  do_check_eq(node.itemId, nonLivemarkItem);
  rootNode.containerOpen = false;

  
  options = histsvc.getNewQueryOptions();
  options.excludeItemIfParentHasAnnotation = "livemark/feedURI";
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  query = histsvc.getNewQuery();
  query.searchTerms = "item";
  result = histsvc.executeQuery(query, options);
  rootNode = result.root;
  rootNode.containerOpen = true;
  cc = rootNode.childCount;
  do_check_eq(cc, 1);
  var node = rootNode.getChild(0);
  do_check_eq(node.itemId, nonLivemarkItem);
  rootNode.containerOpen = false;
}
