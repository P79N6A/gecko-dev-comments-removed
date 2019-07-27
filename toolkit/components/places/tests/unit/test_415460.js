





var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);








function search_has_result(aTerms)
{
  var options = hs.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI;
  var query = hs.getNewQuery();
  query.searchTerms = aTerms;
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
}

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  const SEARCH_TERM = "ユニコード";
  const TEST_URL = "http://example.com/" + SEARCH_TERM + "/";
  yield PlacesTestUtils.addVisits(uri(TEST_URL));
  do_check_true(search_has_result(SEARCH_TERM));
});
