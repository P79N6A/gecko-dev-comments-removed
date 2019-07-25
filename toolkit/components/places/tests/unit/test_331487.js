









































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);


function run_test() {
  
  var folder = bmsvc.createFolder(bmsvc.placesRoot, "test folder", bmsvc.DEFAULT_INDEX);

  
  var b1 = bmsvc.insertBookmark(folder, uri("http://a1.com/"),
                                bmsvc.DEFAULT_INDEX, "1 title");
  
  var sf1 = bmsvc.createFolder(folder, "subfolder 1", bmsvc.DEFAULT_INDEX);

  
  var b2 = bmsvc.insertBookmark(sf1, uri("http://a2.com/"),
                                bmsvc.DEFAULT_INDEX, "2 title");

  
  var sf2 = bmsvc.createFolder(sf1, "subfolder 2", bmsvc.DEFAULT_INDEX);

  
  var b3 = bmsvc.insertBookmark(sf2, uri("http://a3.com/"),
                                bmsvc.DEFAULT_INDEX, "3 title");

  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var query = histsvc.getNewQuery();
  query.setFolders([folder], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 2);
  do_check_eq(root.getChild(0).itemId, b1);
  do_check_eq(root.getChild(1).itemId, sf1);

  
  var sf1Node = root.getChild(1);
  sf1Node = sf1Node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  sf1Node.containerOpen = true;
  do_check_eq(sf1Node.childCount, 2);
  do_check_eq(sf1Node.getChild(0).itemId, b2);
  do_check_eq(sf1Node.getChild(1).itemId, sf2);

  
  var sf2Node = sf1Node.getChild(1);
  sf2Node = sf2Node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  sf2Node.containerOpen = true;
  do_check_eq(sf2Node.childCount, 1);
  do_check_eq(sf2Node.getChild(0).itemId, b3);

  sf2Node.containerOpen = false;
  sf1Node.containerOpen = false;
  root.containerOpen = false;

  
  
  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 10;
  var query = histsvc.getNewQuery();
  query.setFolders([folder], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 3);
  do_check_eq(root.getChild(0).itemId, b1);
  do_check_eq(root.getChild(1).itemId, b2);
  do_check_eq(root.getChild(2).itemId, b3);
  root.containerOpen = false;

  
  
  
  
  
}
