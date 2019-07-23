






































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


var testRoot = bmsvc.bookmarksMenuFolder;


function run_test() {
  var fz = bmsvc.createFolder(testRoot, "fz", bmsvc.DEFAULT_INDEX);
  var fzb1 = bmsvc.insertBookmark(fz, uri("http://a1.com/"),
                                bmsvc.DEFAULT_INDEX, "1 title");

  var fa = bmsvc.createFolder(testRoot, "fa", bmsvc.DEFAULT_INDEX);
  var fab1 = bmsvc.insertBookmark(fa, uri("http://a.a1.com/"),
                                bmsvc.DEFAULT_INDEX, "1 title");

  var f2 = bmsvc.createFolder(testRoot, "f2", bmsvc.DEFAULT_INDEX);
  var f2b2 = bmsvc.insertBookmark(f2, uri("http://a2.com/"),
                                bmsvc.DEFAULT_INDEX, "2 title");
  var f2b3 = bmsvc.insertBookmark(f2, uri("http://a3.com/"),
                                bmsvc.DEFAULT_INDEX, "3 title");

  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
  options.applyOptionsToContainers = true;
  query.setFolders([testRoot], 1);

  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  result.sortingMode = options.SORT_BY_COUNT_ASCENDING;
  do_check_eq(rootNode.childCount, 3);
  do_check_eq(rootNode.getChild(0).title, "fa");
  do_check_eq(rootNode.getChild(1).title, "fz");
  do_check_eq(rootNode.getChild(2).title, "f2");

  
  var rfNode = rootNode.getChild(0);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 1);
  do_check_eq(rfNode.getChild(0).itemId, fab1);

  rfNode = rootNode.getChild(1);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 1);
  do_check_eq(rfNode.getChild(0).itemId, fzb1);

  rfNode = rootNode.getChild(2);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 2);
  do_check_eq(rfNode.getChild(0).itemId, f2b2);
  do_check_eq(rfNode.getChild(1).itemId, f2b3);

  
  result.sortingMode = options.SORT_BY_COUNT_DESCENDING;
  do_check_eq(rootNode.childCount, 3);
  do_check_eq(rootNode.getChild(0).title, "f2");
  do_check_eq(rootNode.getChild(1).title, "fz");
  do_check_eq(rootNode.getChild(2).title, "fa");

  
  rfNode = rootNode.getChild(0);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 2);
  do_check_eq(rfNode.getChild(0).itemId, f2b3);
  do_check_eq(rfNode.getChild(1).itemId, f2b2);

  rfNode = rootNode.getChild(1);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 1);
  do_check_eq(rfNode.getChild(0).itemId, fzb1);

  rfNode = rootNode.getChild(2);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 1);
  do_check_eq(rfNode.getChild(0).itemId, fab1);

  
  
  options = histsvc.getNewQueryOptions();
  query = histsvc.getNewQuery();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
  options.maxResults = 2;
  options.applyOptionsToContainers = true;
  query.setFolders([testRoot], 1);

  result = histsvc.executeQuery(query, options);
  result.sortingMode = options.SORT_BY_COUNT_DESCENDING;

  rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 2);
  do_check_eq(rootNode.getChild(0).title, "f2");
  do_check_eq(rootNode.getChild(1).title, "fz");

  
  rfNode = rootNode.getChild(0);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 2);
  do_check_eq(rfNode.getChild(0).itemId, f2b3);
  do_check_eq(rfNode.getChild(1).itemId, f2b2);

  rfNode = rootNode.getChild(1);
  rfNode = rfNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  rfNode.containerOpen = true;
  do_check_eq(rfNode.childCount, 1);
  do_check_eq(rfNode.getChild(0).itemId, fzb1);
}
