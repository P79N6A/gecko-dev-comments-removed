





































function run_test() {
  try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  } catch(ex) {
    do_throw("Unable to initialize Places services");
  }

  
  var queryId = bmsvc.insertBookmark(bmsvc.toolbarFolder, uri("place:"),
                                     0 , "test query");

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.toolbarFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var queryNode = root.getChild(0);
  do_check_eq(queryNode.title, "test query");

  
  bmsvc.setItemTitle(queryId, "foo");

  
  do_check_eq(queryNode.title, "foo");

  root.containerOpen = false;
}
