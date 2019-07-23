









































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function add_visit(aURI, aWhen, aType) {
  var placeID = histsvc.addVisit(aURI,
                                 aWhen,
                                 0, 
                                 aType,
                                 false, 
                                 0);
  do_check_true(placeID > 0);
  return placeID;
}

const TOTAL_SITES = 20;


function run_test() {
  var now = Date.now();
  
  for (var i=0; i < TOTAL_SITES; i++) {
    var site = "http://www.test-" + i + ".com/";
    var testURI = uri(site);
    var testImageURI = uri(site + "blank.gif");
    var when = now + (i * TOTAL_SITES);
    add_visit(testURI, when, histsvc.TRANSITION_TYPED);
    add_visit(testImageURI, when + 1, histsvc.TRANSITION_EMBED);
    add_visit(testURI, when + 2, histsvc.TRANSITION_LINK);
  }

  
  
  
  
  
  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  options.includeHidden = true;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, 3 * TOTAL_SITES); 
  for (var i=0; i < TOTAL_SITES; i++) {
    var node = root.getChild(i*3);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(i*3+1);
    do_check_eq(node.uri, site + "blank.gif");
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(i*3+2);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  
  do_check_eq(cc, 2 * TOTAL_SITES); 
  for (var i=0; i < TOTAL_SITES; i++) {
    var node = root.getChild(i*2);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(i*2+1);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
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
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
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
