





































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var gh = hs.QueryInterface(Ci.nsIGlobalHistory2);








function uri_in_db(aURI) {
  var options = hs.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI
  var query = hs.getNewQuery();
  query.uri = aURI;
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
}


function run_test() {
  
  var file = do_get_file("history_import_test.dat");
  gh.importHistory(file);
  var uri1 = uri("http://www.mozilla.org/");
  do_check_true(uri_in_db(uri1));

  
  var options = hs.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  var query = hs.getNewQuery();
  query.minVisits = 4;
  query.maxVisits = 4;
  query.uri = uri1;
  var result = hs.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, 4);

  
  var lastVisitDate = root.getChild(0).time;
  do_check_eq(lastVisitDate, 1230666859910484);
  var firstVisitDate = root.getChild(3).time;
  do_check_eq(firstVisitDate, 1230666845910353);

  
  do_check_true(root.getChild(1).time < lastVisitDate &&
                root.getChild(1).time > firstVisitDate);
  do_check_true(root.getChild(2).time < lastVisitDate &&
                root.getChild(2).time > firstVisitDate);
  do_check_true(root.getChild(1).time != root.getChild(2).time);

  root.containerOpen = false;
}
