













var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
getService(Ci.nsINavHistoryService);

function run_test()
{
  run_next_test();
}

add_task(function test_autocomplete_on_value_removed()
{
  
  var listener = Cc["@mozilla.org/autocomplete/search;1?name=history"].
                 getService(Components.interfaces.nsIAutoCompleteSimpleResultListener);

  
  var testUri = uri("http://foo.mozilla.com/");
  yield PlacesTestUtils.addVisits({
    uri: testUri,
    referrer: uri("http://mozilla.com/")
  });
  
  var query = histsvc.getNewQuery();
  
  var options = histsvc.getNewQueryOptions();
  
  query.uri = testUri;
  
  var queryRes = histsvc.executeQuery(query, options);
  
  queryRes.root.containerOpen = true;
  
  
  do_check_eq(queryRes.root.childCount, 1);  
  
  listener.onValueRemoved(null, testUri.spec, true);
  
  do_check_eq(queryRes.root.childCount, 0);
  
  queryRes.root.containerOpen = false;
});
