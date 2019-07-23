






































var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

function check_queries_results(aQueries, aOptions, aExpectedItemIds) {
  var result = hs.executeQueries(aQueries, aQueries.length, aOptions);
  var root = result.root;
  root.containerOpen = true;

  
  for (let i = 0; i < root.childCount; i++) {
    dump("nodes[" + i + "]: " + root.getChild(0).title + "\n");
  }

  do_check_eq(root.childCount, aExpectedItemIds.length);
  for (let i = 0; i < root.childCount; i++) {
    do_check_eq(root.getChild(i).itemId, aExpectedItemIds[i]);
  }

  root.containerOpen = false;
}


function run_test() {
  var id1 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "123 0");
  var id2 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "456");
  var id3 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "123 456");
  var id4 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "789 456");

  



  var queries = [];
  queries.push(hs.getNewQuery());
  queries.push(hs.getNewQuery());
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;

  
  dump("Test searching for 123 OR 789\n");
  queries[0].searchTerms = "123";
  queries[1].searchTerms = "789";
  check_queries_results(queries, options, [id1, id3, id4]);

  
  dump("Test searching for 123 OR 456\n");
  queries[0].searchTerms = "123";
  queries[1].searchTerms = "456";
  check_queries_results(queries, options, [id1, id2, id3, id4]);

  
  dump("Test searching for 00 OR 789\n");
  queries[0].searchTerms = "00";
  queries[1].searchTerms = "789";
  check_queries_results(queries, options, [id4]);
}
