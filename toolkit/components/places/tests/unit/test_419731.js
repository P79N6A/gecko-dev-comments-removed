







































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
  
  tagssvc.tagURI(uri1, ["foo"]);

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.tagsFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var tagRoot = result.root;
  tagRoot.containerOpen = true;
  var tagNode = tagRoot.getChild(0)
                       .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  var tagItemId = tagNode.itemId;
  tagRoot.containerOpen = false;

  
  bmsvc.setItemTitle(bookmark1id, "new title 1");

  
  options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.resultType = options.RESULTS_AS_TAG_CONTENTS;

  query = histsvc.getNewQuery();
  query.setFolders([tagItemId], 1);
  result = histsvc.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, 1);
  var node = root.getChild(0);
  do_check_eq(node.title, "new title 1");
  root.containerOpen = false;

  
  bmsvc.setItemTitle(bookmark2id, "new title 2");
  
  var bookmark1LastMod = bmsvc.getItemLastModified(bookmark1id);
  bmsvc.setItemLastModified(bookmark2id, bookmark1LastMod + 1);

  
  options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.resultType = options.RESULTS_AS_TAG_CONTENTS;

  query = histsvc.getNewQuery();
  query.setFolders([tagItemId], 1);
  result = histsvc.executeQuery(query, options);
  root = result.root;

  root.containerOpen = true;
  cc = root.childCount;
  do_check_eq(cc, 1);
  node = root.getChild(0);
  do_check_eq(node.title, "new title 2");
  root.containerOpen = false;
}
