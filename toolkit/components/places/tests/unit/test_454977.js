






































try {
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var mDBConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
} catch(ex) {
  do_throw("Could not get services\n");
}


var visit_count = 0;

function add_visit(aURI, aVisitDate, aVisitType) {
  var isRedirect = aVisitType == hs.TRANSITION_REDIRECT_PERMANENT ||
                   aVisitType == hs.TRANSITION_REDIRECT_TEMPORARY;
  var visitId = hs.addVisit(aURI, aVisitDate, null,
                            aVisitType, isRedirect, 0);
  do_check_true(visitId > 0);
  
  if (aVisitType != 0 &&
      aVisitType != hs.TRANSITION_EMBED &&
      aVisitType != hs.TRANSITION_DOWNLOAD)
    visit_count ++;
  
  var sql = "SELECT place_id FROM moz_historyvisits_view WHERE id = ?1";
  var stmt = mDBConn.createStatement(sql);
  stmt.bindInt64Parameter(0, visitId);
  do_check_true(stmt.executeStep());
  var placeId = stmt.getInt64(0);
  stmt.finalize();
  do_check_true(placeId > 0);
  return placeId;
}








function check_results(aExpectedCount, aExpectedCountWithHidden) {
  var query = hs.getNewQuery();
  
  query.minVisits = visit_count;
  query.maxVisits = visit_count;
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  
  do_check_eq(root.childCount, aExpectedCount);
  root.containerOpen = false;

  
  
  options.includeHidden = true;
  result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  
  do_check_eq(root.childCount, aExpectedCountWithHidden);
  root.containerOpen = false;
}


function run_test() {
  var testURI = uri("http://test.mozilla.org/");

  
  var placeId = add_visit(testURI, Date.now()*1000, hs.TRANSITION_EMBED);
  check_results(0, 1);

  
  
  
  do_check_eq(add_visit(testURI, Date.now()*1000, hs.TRANSITION_TYPED), placeId);
  check_results(1, 1);

  
  
  
  do_check_eq(add_visit(testURI, Date.now()*1000, hs.TRANSITION_EMBED), placeId);
  check_results(1, 1);
}
