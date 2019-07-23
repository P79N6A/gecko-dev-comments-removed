







































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


var root = bmsvc.bookmarksMenuFolder;


function run_test() {
  
  var testFolder1 = bmsvc.createFolder(root, "bug 384228 test folder 1",
                                       bmsvc.DEFAULT_INDEX);
  var testFolder2 = bmsvc.createFolder(root, "bug 384228 test folder 2",
                                       bmsvc.DEFAULT_INDEX);
  var testFolder3 = bmsvc.createFolder(root, "bug 384228 test folder 3",
                                       bmsvc.DEFAULT_INDEX);

  var b1 = bmsvc.insertBookmark(testFolder1, uri("http://foo.tld/"),
                                bmsvc.DEFAULT_INDEX, "title b1 (folder 1)");
  var b2 = bmsvc.insertBookmark(testFolder1, uri("http://foo.tld/"),
                                bmsvc.DEFAULT_INDEX, "title b2 (folder 1)");
  var b3 = bmsvc.insertBookmark(testFolder2, uri("http://foo.tld/"),
                                bmsvc.DEFAULT_INDEX, "title b3 (folder 2)");
  var b4 = bmsvc.insertBookmark(testFolder3, uri("http://foo.tld/"),
                                bmsvc.DEFAULT_INDEX, "title b4 (folder 3)");
  
  var testFolder1_1 = bmsvc.createFolder(testFolder1, "bug 384228 test folder 1.1",
                                         bmsvc.DEFAULT_INDEX);
  var b5 = bmsvc.insertBookmark(testFolder1_1, uri("http://a1.com/"),
                                bmsvc.DEFAULT_INDEX, "title b5 (folder 1.1)");
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.searchTerms = "title";
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  query.setFolders([testFolder1, testFolder2], 2);

  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  do_check_eq(rootNode.childCount, 4);

  do_check_eq(rootNode.getChild(0).itemId, b1);
  do_check_eq(rootNode.getChild(1).itemId, b2);
  do_check_eq(rootNode.getChild(2).itemId, b3);
  do_check_eq(rootNode.getChild(3).itemId, b5);
}
