





































function run_test() {
  try {
    var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);
    var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  } catch(ex) {
    do_throw("Unable to initialize Places services");
  }


  
  var testURI = uri("http://test");
  histsvc.addVisit(testURI, Date.now() * 1000, null,
                   histsvc.TRANSITION_TYPED, false, 0);

  
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI;
  var query = histsvc.getNewQuery();
  query.uri = testURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;

  
  do_check_eq(root.hasChildren, true);

  
  var queryURI = histsvc.queriesToQueryString([query], 1, options);
  bmsvc.insertBookmark(bmsvc.toolbarFolder, uri(queryURI),
                       0 , "test query");

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.toolbarFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var queryNode = root.getChild(0);
  do_check_eq(queryNode.title, "test query");
  queryNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(queryNode.hasChildren, true);
  root.containerOpen = false;
}
