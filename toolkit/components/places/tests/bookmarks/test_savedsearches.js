







































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
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


var root = bmsvc.bookmarksMenuFolder;


function run_test() {
  
  var searchTerm = "about";

  
  
  
  var testRoot = bmsvc.createFolder(root, searchTerm, bmsvc.DEFAULT_INDEX);

  



  
  var bookmarkId = bmsvc.insertBookmark(root, uri("http://foo.com"), bmsvc.DEFAULT_INDEX, searchTerm);

  
  var searchId = bmsvc.insertBookmark(testRoot,
                                      uri("place:terms=" + searchTerm + "&excludeQueries=1&expandQueries=1&queryType=1"),
                                      bmsvc.DEFAULT_INDEX, searchTerm);

  
  
  try {
    var options = histsvc.getNewQueryOptions();
    options.expandQueries = 0;
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_true(node.itemId > 0);
      
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      do_check_eq(node.containerOpen, false);
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=0 query error: " + ex);
  }

  
  
  
  try {
    var options = histsvc.getNewQueryOptions();
    options.expandQueries = 1;
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_eq(node.type, node.RESULT_TYPE_QUERY);
      
      do_check_true(node.itemId > 0);
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      node.containerOpen = true;

      
      
      
      do_check_eq(node.childCount, 1);

      
      var item = node.getChild(0);
      do_check_eq(item.itemId, bookmarkId);

      
      
      

      
      
      

      
      bmsvc.createFolder(root, searchTerm + "zaa", bmsvc.DEFAULT_INDEX);
      do_check_eq(node.childCount, 1);
      
      bmsvc.insertBookmark(root, uri("place:terms=foo&excludeQueries=1&expandQueries=1&queryType=1"),
                           bmsvc.DEFAULT_INDEX, searchTerm + "blah");
      do_check_eq(node.childCount, 1);
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=1 bookmarks query: " + ex);
  }

  
  bmsvc.removeItem(searchId);

  



  
  var testURI = uri("http://" + searchTerm + ".com");
  bhist.addPageWithDetails(testURI, searchTerm, Date.now() * 1000);

  
  var searchId = bmsvc.insertBookmark(testRoot,
                                      uri("place:terms=" + searchTerm + "&excludeQueries=1&expandQueries=1&queryType=0"),
                                      bmsvc.DEFAULT_INDEX, searchTerm);

  
  
  try {
    var options = histsvc.getNewQueryOptions();
    options.expandQueries = 1;
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_eq(node.type, node.RESULT_TYPE_QUERY);
      
      do_check_eq(node.itemId, searchId);
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      node.containerOpen = true;

      
      
      
      do_check_eq(node.childCount, 1);

      
      var item = node.getChild(0);
      do_check_eq(item.type, item.RESULT_TYPE_URI);
      do_check_eq(item.itemId, -1); 
      do_check_eq(item.uri, testURI.spec); 

      
      bhist.addPageWithDetails(uri("http://foo.com"), searchTerm + "blah", Date.now() * 1000);
      do_check_eq(node.childCount, 2);

      
      bhist.removePage(uri("http://foo.com"));
      do_check_eq(node.childCount, 1);
      node.containerOpen = false;
    }

    
    var tmpFolderId = bmsvc.createFolder(testRoot, "foo", bmsvc.DEFAULT_INDEX); 
    bmsvc.moveItem(searchId, tmpFolderId, bmsvc.DEFAULT_INDEX);
    var tmpFolderNode = rootNode.getChild(0);
    do_check_eq(tmpFolderNode.itemId, tmpFolderId);
    tmpFolderNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    tmpFolderNode.containerOpen = true;
    do_check_eq(tmpFolderNode.childCount, 1);

    
    bmsvc.setItemTitle(searchId, "foo");
    do_check_eq(tmpFolderNode.title, "foo");

    
    bmsvc.removeItem(searchId);
    try {
      var tmpFolderNode = root.getChild(1);
      do_throw("query was not removed");
    } catch(ex) {}

    tmpFolderNode.containerOpen = false;
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=1 bookmarks query: " + ex);
  }
}
