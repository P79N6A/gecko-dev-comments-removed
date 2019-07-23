













































var ios = Cc["@mozilla.org/network/io-service;1"].
getService(Components.interfaces.nsIIOService);

var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
getService(Ci.nsINavHistoryService);

function run_test()
{
  
  var historyService = histsvc.QueryInterface(Components.interfaces.nsIAutoCompleteSimpleResultListener);

  
  var now = Date.now() * 1000;
  var uri = ios.newURI("http://foo.mozilla.com/", null, null);
  var ref = ios.newURI("http://mozilla.com/", null, null);
  var visit = histsvc.addVisit(uri, now, ref, 1, false, 0);
  
  var query = historyService.getNewQuery();
  
  var options = historyService.getNewQueryOptions();
  
  query.uri = uri;
  
  var queryRes = historyService.executeQuery(query, options);
  
  queryRes.root.containerOpen = true;
  
  
  do_check_eq(queryRes.root.childCount, 1);  
  
  historyService.onValueRemoved(null, uri.spec, true);
  
  do_check_eq(queryRes.root.childCount, 0);
  
  queryRes.root.containerOpen = false;
}

