













































var ios = Cc["@mozilla.org/network/io-service;1"].
getService(Components.interfaces.nsIIOService);

var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
getService(Ci.nsINavHistoryService);

function run_test()
{
  
  var listener = Cc["@mozilla.org/autocomplete/search;1?name=history"].
                 getService(Components.interfaces.nsIAutoCompleteSimpleResultListener);

  
  var now = Date.now() * 1000;
  var uri = ios.newURI("http://foo.mozilla.com/", null, null);
  var ref = ios.newURI("http://mozilla.com/", null, null);
  var visit = histsvc.addVisit(uri, now, ref, 1, false, 0);
  
  var query = histsvc.getNewQuery();
  
  var options = histsvc.getNewQueryOptions();
  
  query.uri = uri;
  
  var queryRes = histsvc.executeQuery(query, options);
  
  queryRes.root.containerOpen = true;
  
  
  do_check_eq(queryRes.root.childCount, 1);  
  
  listener.onValueRemoved(null, uri.spec, true);
  
  do_check_eq(queryRes.root.childCount, 0);
  
  queryRes.root.containerOpen = false;
}

