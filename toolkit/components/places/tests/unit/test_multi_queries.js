






































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);










function add_visit(aURI, aDayOffset, aTransition) {
  var visitID = histsvc.addVisit(aURI,
                                 (Date.now() + aDayOffset*86400000) * 1000,
                                 null,
                                 aTransition,
                                 false, 
                                 0);
  do_check_true(visitID > 0);
  return visitID;
}


function run_test() {
   var testURI = uri("http://mirror1.mozilla.com/a");
  add_visit(testURI, -1, histsvc.TRANSITION_LINK);
  testURI = uri("http://mirror2.mozilla.com/b");
  add_visit(testURI, -2, histsvc.TRANSITION_LINK);
  testURI = uri("http://mirror3.mozilla.com/c");
  add_visit(testURI, -4, histsvc.TRANSITION_FRAMED_LINK);
  testURI = uri("http://mirror1.google.com/b");
  add_visit(testURI, -1, histsvc.TRANSITION_EMBED);
  testURI = uri("http://mirror2.google.com/a");
  add_visit(testURI, -2, histsvc.TRANSITION_LINK);
  testURI = uri("http://mirror1.apache.org/b");
  add_visit(testURI, -3, histsvc.TRANSITION_LINK);
  testURI = uri("http://mirror2.apache.org/a");
  add_visit(testURI, -4, histsvc.TRANSITION_FRAMED_LINK);


  var options = histsvc.getNewQueryOptions();
  var queries = [];
  queries.push(histsvc.getNewQuery());
  queries.push(histsvc.getNewQuery());

  queries[0].domain = "mozilla.com";
  queries[1].domain = "google.com";

  var result = histsvc.executeQueries(queries, queries.length, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 3);
  root.containerOpen = false;
}
