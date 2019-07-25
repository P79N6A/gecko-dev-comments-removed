






































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


try {
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}


var root = bmsvc.bookmarksMenuFolder;


function run_test() {
  

  
  var folder = bmsvc.createFolder(root, "bug 395101 test", bmsvc.DEFAULT_INDEX);

  
  var testURI = uri("http://a1.com");
  var b1 = bmsvc.insertBookmark(folder, testURI,
                                bmsvc.DEFAULT_INDEX, "1 title");

  
  tagssvc.tagURI(testURI, ["elephant", "walrus", "giraffe", "turkey", "hiPPo", "BABOON", "alf"]);

  
  var query = histsvc.getNewQuery();
  query.searchTerms = "elephant";
  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  query.setFolders([folder], 1);

  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;

  do_check_eq(rootNode.childCount, 1);
  do_check_eq(rootNode.getChild(0).itemId, b1);
  rootNode.containerOpen = false;

  
  query.searchTerms = "wal";
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 1);
  rootNode.containerOpen = false;

  
  query.searchTerms = "WALRUS";
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 1);
  do_check_eq(rootNode.getChild(0).itemId, b1);
  rootNode.containerOpen = false;

  
  query.searchTerms = "baboon";
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 1);
  do_check_eq(rootNode.getChild(0).itemId, b1);
  rootNode.containerOpen = false;
}
