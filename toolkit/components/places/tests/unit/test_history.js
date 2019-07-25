






var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var gh = histsvc.QueryInterface(Ci.nsIGlobalHistory2);
var bh = histsvc.QueryInterface(Ci.nsIBrowserHistory);










function add_visit(aURI, aReferrer) {
  var visitId = histsvc.addVisit(aURI,
                                 Date.now() * 1000,
                                 aReferrer,
                                 histsvc.TRANSITION_TYPED, 
                                 false, 
                                 0);
  dump("### Added visit with id of " + visitId + "\n");
  do_check_true(gh.isVisited(aURI));
  do_check_guid_for_uri(aURI);
  return visitId;
}








function uri_in_db(aURI) {
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI
  var query = histsvc.getNewQuery();
  query.uri = aURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
}


function run_test() {
  
  do_check_eq(histsvc.databaseStatus, histsvc.DATABASE_STATUS_CREATE);

  
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
  result.root.containerOpen = false;
  query.minVisits = 1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  result.root.containerOpen = false;
  query.minVisits = 2;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  query.minVisits = 3;
  result.root.containerOpen = false;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 0);
  result.root.containerOpen = false;

  
  query.minVisits = -1;
  query.maxVisits = -1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  result.root.containerOpen = false;
  query.maxVisits = 0;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 0);
  result.root.containerOpen = false;
  query.maxVisits = 1;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  result.root.containerOpen = false;
  query.maxVisits = 2;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  result.root.containerOpen = false;
  query.maxVisits = 3;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 2);
  result.root.containerOpen = false;
  
  
  var annos = Cc["@mozilla.org/browser/annotation-service;1"].
              getService(Ci.nsIAnnotationService);
  annos.setPageAnnotation(uri("http://mozilla.com/"), "testAnno", 0, 0,
                          Ci.nsIAnnotationService.EXPIRE_NEVER);
  query.annotation = "testAnno";
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  do_check_eq(result.root.getChild(0).uri, "http://mozilla.com/");
  result.root.containerOpen = false;

  
  query.annotationIsNot = true;
  result = histsvc.executeQuery(query, options);
  result.root.containerOpen = true;
  do_check_eq(result.root.childCount, 1);
  do_check_eq(result.root.getChild(0).uri, "http://google.com/");
  result.root.containerOpen = false;

  
  do_check_true(!histsvc.historyDisabled);

  
  var title = histsvc.getPageTitle(uri("http://mozilla.com"));
  do_check_eq(title, null);

  
  do_check_true(uri_in_db(testURI));

  
  
  var db = histsvc.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
  var q = "SELECT id FROM moz_bookmarks";
  var statement;
  try {
     statement = db.createStatement(q);
  } catch(ex) {
    do_throw("bookmarks table does not have id field, schema is too old!");
  }
  finally {
    statement.finalize();
  }

  
  add_visit(uri("http://mozilla.com"));
  var options = histsvc.getNewQueryOptions();
  
  var query = histsvc.getNewQuery();
  query.searchTerms = "moz";
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_true(root.childCount > 0);
  root.containerOpen = false;

  
  var referrerURI = uri("http://yahoo.com");
  do_check_false(uri_in_db(referrerURI));
  add_visit(uri("http://mozilla.com"), referrerURI);
  do_check_true(uri_in_db(referrerURI));
}
