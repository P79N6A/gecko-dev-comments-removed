






































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


var root = bmsvc.bookmarksRoot;


function run_test() {
  



  var id1 = bmsvc.insertBookmark(root, uri("http://foo.tld"),
                                 bmsvc.DEFAULT_INDEX, "123 0");
  var id2 = bmsvc.insertBookmark(root, uri("http://foo.tld"),
                                 bmsvc.DEFAULT_INDEX, "456");
  var id3 = bmsvc.insertBookmark(root, uri("http://foo.tld"),
                                 bmsvc.DEFAULT_INDEX, "123 456");
  var id4 = bmsvc.insertBookmark(root, uri("http://foo.tld"),
                                 bmsvc.DEFAULT_INDEX, "789 456");

  var queries = [];
  queries.push(histsvc.getNewQuery());
  queries[0].searchTerms = "123";
  queries.push(histsvc.getNewQuery());
  queries[1].searchTerms = "789";

  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;

  var result = histsvc.executeQueries(queries, queries.length, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 3);
  do_check_eq(root.getChild(0).itemId, id1);
  do_check_eq(root.getChild(1).itemId, id3);
  do_check_eq(root.getChild(2).itemId, id4);

  queries[0].searchTerms = "123";
  queries[1].searchTerms = "456";
  result = histsvc.executeQueries(queries, queries.length, options);
  root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 4);
  do_check_eq(root.getChild(0).itemId, id1);
  do_check_eq(root.getChild(1).itemId, id2);
  do_check_eq(root.getChild(2).itemId, id3);
  do_check_eq(root.getChild(3).itemId, id4);

  queries[0].searchTerms = "00";
  queries[1].searchTerms = "789";
  result = histsvc.executeQueries(queries, queries.length, options);
  root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 1);
  do_check_eq(root.getChild(0).itemId, id4);
}
