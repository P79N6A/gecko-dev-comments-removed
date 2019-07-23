








































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
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

  tagssvc.tagURI(uri1, ["tag 1"]);
  tagssvc.tagURI(uri2, ["tag 2"]);

  histsvc.addVisit(uri1, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  histsvc.setPageDetails(uri1, "foo title", 0, false, true);
  histsvc.addVisit(uri2, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  histsvc.setPageDetails(uri2, "bar title", 0, false, true);

  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 2;

  options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 2;

  query = histsvc.getNewQuery();
  query.setFolders([bmsvc.tagsFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 2);
  do_check_eq(root.getChild(0).title, "foo title");
  do_check_eq(root.getChild(1).title, "bar title");
}
