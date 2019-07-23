









































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);


function add_visit(aURI, aWhen, aType) {
  var visitID = histsvc.addVisit(aURI,
                                 aWhen,
                                 null, 
                                 aType,
                                 false, 
                                 0);
  do_check_true(visitID > 0);
  return visitID;
}

const TOTAL_SITES = 20;


function run_test() {
  var now = Date.now() * 1000;
  
  for (var i=0; i < TOTAL_SITES; i++) {
    var site = "http://www.test-" + i + ".com/";
    var testURI = uri(site);
    var testImageURI = uri(site + "blank.gif");
    var when = now + (i * TOTAL_SITES);
    add_visit(testURI, when, histsvc.TRANSITION_TYPED);
    add_visit(testImageURI, ++when, histsvc.TRANSITION_EMBED);
    add_visit(testImageURI, ++when, histsvc.TRANSITION_FRAMED_LINK);
    add_visit(testURI, ++when, histsvc.TRANSITION_LINK);
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
  do_check_eq(cc, 4 * TOTAL_SITES);
  for (var i = 0; i < TOTAL_SITES; i++) {
    var index = i * 4;
    var node = root.getChild(index);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
    do_check_eq(node.uri, site + "blank.gif");
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
    do_check_eq(node.uri, site + "blank.gif");
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
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
    var index = i * 2;
    var node = root.getChild(index);
    var site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
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
