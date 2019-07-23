









































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function add_visit(aURI, aType) {
  var placeID = histsvc.addVisit(uri(aURI),
                                 Date.now() * 1000,
                                 null, 
                                 aType,
                                 false, 
                                 0);
  do_check_true(placeID > 0);
  return placeID;
}

const TOTAL_SITES = 20;


function run_test() {
  
  for (var i=0; i < TOTAL_SITES; i++) {
    for (var j=0; j<=i; j++) {
      add_visit("http://www.test-" + i + ".com/", histsvc.TRANSITION_TYPED);
      
      
      add_visit("http://www.hidden.com/hidden.gif", histsvc.TRANSITION_EMBED);
      add_visit("http://www.alsohidden.com/hidden.gif", histsvc.TRANSITION_FRAMED_LINK);
    }
  }

  
  
  
  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.maxResults = 10;
  options.resultType = options.RESULTS_AS_URI;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, options.maxResults);
  for (var i=0; i < cc; i++) {
    var node = root.getChild(i);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.resultType = options.RESULTS_AS_URI;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, TOTAL_SITES);
  for (var i=0; i < 10; i++) {
    var node = root.getChild(i);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_URI);
  }
  root.containerOpen = false;
}
