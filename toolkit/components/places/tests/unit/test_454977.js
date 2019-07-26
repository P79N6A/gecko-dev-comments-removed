






let visit_count = 0;


function task_add_visit(aURI, aVisitType)
{
  
  let deferUpdatePlaces = Promise.defer();
  PlacesUtils.asyncHistory.updatePlaces({
    uri: aURI,
    visits: [{ transitionType: aVisitType, visitDate: Date.now() * 1000 }]
  }, {
    handleError: function TAV_handleError() {
      deferUpdatePlaces.reject(new Error("Unexpected error in adding visit."));
    },
    handleResult: function (aPlaceInfo) {
      this.visitId = aPlaceInfo.visits[0].visitId;
    },
    handleCompletion: function TAV_handleCompletion() {
      deferUpdatePlaces.resolve(this.visitId);
    }
  });
  let visitId = yield deferUpdatePlaces.promise;

  
  if (aVisitType != 0 &&
      aVisitType != TRANSITION_EMBED &&
      aVisitType != TRANSITION_FRAMED_LINK &&
      aVisitType != TRANSITION_DOWNLOAD) {
    visit_count ++;
  }

  
  if (visitId > 0) {
    let sql = "SELECT place_id FROM moz_historyvisits WHERE id = ?1";
    let stmt = DBConn().createStatement(sql);
    stmt.bindByIndex(0, visitId);
    do_check_true(stmt.executeStep());
    let placeId = stmt.getInt64(0);
    stmt.finalize();
    do_check_true(placeId > 0);
    throw new Task.Result(placeId);
  }
  throw new Task.Result(0);
}








function check_results(aExpectedCount, aExpectedCountWithHidden)
{
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


function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  const TEST_URI = uri("http://test.mozilla.org/");

  
  yield task_add_visit(TEST_URI, TRANSITION_EMBED);
  check_results(0, 0);

  let placeId = yield task_add_visit(TEST_URI, TRANSITION_FRAMED_LINK);
  check_results(0, 1);

  
  
  
  do_check_eq((yield task_add_visit(TEST_URI, TRANSITION_TYPED)), placeId);
  check_results(1, 1);

  
  
  
  yield task_add_visit(TEST_URI, TRANSITION_EMBED);
  check_results(1, 1);
});
