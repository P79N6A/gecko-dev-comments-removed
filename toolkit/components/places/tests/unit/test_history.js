








































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function add_visit(aURI) {
  var placeID = histsvc.addVisit(aURI,
                                 Date.now(),
                                 0, 
                                 histsvc.TRANSITION_TYPED, 
                                 false, 
                                 0);
  do_check_true(placeID > 0);
  return placeID;
}


function run_test() {
  
  var testURI = uri("http://mozilla.com");
  add_visit(testURI);

  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.maxResults = 1;
  
  
  options.resultType = options.RESULTS_AS_VISIT;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  for (var i=0; i < cc; ++i) {
    var node = root.getChild(i);
    
    do_check_eq(node.uri, testURI.spec);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    
    
  }
  root.containerOpen = false;

  
  var testURI2 = uri("http://google.com/");
  add_visit(testURI);
  add_visit(testURI2);

  options.maxResults = 5;
  options.resultType = options.RESULTS_AS_URI;

  
  query.minVisits = 0;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  query.minVisits = 1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  query.minVisits = 2;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  query.minVisits = 3;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 0);

  
  query.minVisits = -1;
  query.maxVisits = -1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  query.maxVisits = 0;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 0);
  query.maxVisits = 1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  query.maxVisits = 2;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  query.maxVisits = 3;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);

  
  do_check_true(!histsvc.historyDisabled);

  
  var title = histsvc.getPageTitle(uri("http://mozilla.com"));
  do_check_eq(title, "mozilla.com");

  

  
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI
  var query = histsvc.getNewQuery();
  query.uri = testURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 1);
  root.containerOpen = false;
}
