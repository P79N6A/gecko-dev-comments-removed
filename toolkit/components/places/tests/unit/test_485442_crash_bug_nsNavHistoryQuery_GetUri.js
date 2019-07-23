





































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

function run_test() {
  var query = hs.getNewQuery();
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULT_TYPE_QUERY;
  var result = hs.executeQuery(query, options);
  result.root.containerOpen = true;
  var rootNode = result.root;
  rootNode.QueryInterface(Ci.nsINavHistoryQueryResultNode);
  var queries = rootNode.getQueries();
  do_check_eq(queries[0].uri, null); 
  rootNode.containerOpen = false;
}
