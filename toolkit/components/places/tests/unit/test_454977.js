






































let visit_count = 0;

function add_visit(aURI, aVisitDate, aVisitType) {
  let isRedirect = aVisitType == TRANSITION_REDIRECT_PERMANENT ||
                   aVisitType == TRANSITION_REDIRECT_TEMPORARY;
  let visitId = PlacesUtils.history.addVisit(aURI, aVisitDate, null,
                                             aVisitType, isRedirect, 0);

  
  if (aVisitType != 0 &&
      aVisitType != TRANSITION_EMBED &&
      aVisitType != TRANSITION_FRAMED_LINK &&
      aVisitType != TRANSITION_DOWNLOAD) {
    visit_count ++;
  }

  
  if (visitId > 0) {
    let sql = "SELECT place_id FROM moz_historyvisits WHERE id = ?1";
    let stmt = DBConn().createStatement(sql);
    stmt.bindInt64Parameter(0, visitId);
    do_check_true(stmt.executeStep());
    let placeId = stmt.getInt64(0);
    stmt.finalize();
    do_check_true(placeId > 0);
    return placeId;
  }
  return 0;
}








function check_results(aExpectedCount, aExpectedCountWithHidden) {
  let query = PlacesUtils.history.getNewQuery();
  
  query.minVisits = visit_count;
  query.maxVisits = visit_count;
  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  
  do_check_eq(root.childCount, aExpectedCount);
  root.containerOpen = false;

  
  
  options.includeHidden = true;
  root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  
  do_check_eq(root.childCount, aExpectedCountWithHidden);
  root.containerOpen = false;
}


function run_test() {
  const TEST_URI = uri("http://test.mozilla.org/");

  
  add_visit(TEST_URI, Date.now()*1000, TRANSITION_EMBED);
  check_results(0, 0);

  let placeId = add_visit(TEST_URI, Date.now()*1000, TRANSITION_FRAMED_LINK);
  check_results(0, 1);

  
  
  
  do_check_eq(add_visit(TEST_URI, Date.now()*1000, TRANSITION_TYPED), placeId);
  check_results(1, 1);

  
  
  
  add_visit(TEST_URI, Date.now()*1000, TRANSITION_EMBED);
  check_results(1, 1);
}
