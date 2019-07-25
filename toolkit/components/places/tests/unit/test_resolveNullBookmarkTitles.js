








































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
  var uri1 = uri("http://foo.tld/");
  var uri2 = uri("https://bar.tld/");

  bhist.addPageWithDetails(uri1, "foo title", Date.now() * 1000);
  bhist.addPageWithDetails(uri2, "bar title", Date.now() * 1000);

  bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri1, bmsvc.DEFAULT_INDEX, null);
  bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri2, bmsvc.DEFAULT_INDEX, null);
  
  tagssvc.tagURI(uri1, ["tag 1"]);
  tagssvc.tagURI(uri2, ["tag 2"]);

  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.resultType = options.RESULTS_AS_TAG_CONTENTS;

  var query = histsvc.getNewQuery();
  
  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 2);
  
  
  do_check_eq(root.getChild(0).title, "bar title");
  do_check_eq(root.getChild(1).title, "foo title");
  root.containerOpen = false;
}
