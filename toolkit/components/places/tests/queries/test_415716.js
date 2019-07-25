





function modHistoryTypes(val){
  switch(val % 8) {
    case 0:
    case 1:
      return TRANSITION_LINK;
    case 2:
      return TRANSITION_TYPED;
    case 3:
      return TRANSITION_BOOKMARK;
    case 4:
      return TRANSITION_EMBED;
    case 5:
      return TRANSITION_REDIRECT_PERMANENT;
    case 6:
      return TRANSITION_REDIRECT_TEMPORARY;
    case 7:
      return TRANSITION_DOWNLOAD;
    case 8:
      return TRANSITION_FRAMED_LINK;
  }
  return TRANSITION_TYPED;
}





function buildTestDatabase() {
  
  
  let testURI = uri("http://www.foo.com");

  PlacesUtils.history.runInBatchMode({
    runBatched: function (aUserData) {
      for (let i = 0; i < 12; ++i) {
        PlacesUtils.history.addVisit(testURI, today, null, modHistoryTypes(i),
                                     false, 0);
      }
      
      testURI = uri("http://foo.com/youdontseeme.html");
      let testAnnoName = "moz-test-places/testing123";
      let testAnnoVal = "test";
      for (let i = 0; i < 12; ++i) {
        PlacesUtils.history.addVisit(testURI, today, null, modHistoryTypes(i),
                                     false, 0);
      }
      PlacesUtils.annotations.setPageAnnotation(testURI, testAnnoName,
                                                testAnnoVal, 0, 0);
    }
  }, null);
}










function run_test() {
  buildTestDatabase();
  let query = PlacesUtils.history.getNewQuery();
  query.annotation = "moz-test-places/testing123";
  query.beginTime = daybefore * 1000;
  query.beginTimeReference = PlacesUtils.history.TIME_RELATIVE_NOW;
  query.endTime = today * 1000;
  query.endTimeReference = PlacesUtils.history.TIME_RELATIVE_NOW;
  query.minVisits = 2;
  query.maxVisits = 10;

  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;

  
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  dump("----> cc is: " + cc + "\n");
  for(let i = 0; i < root.childCount; ++i) {
    let resultNode = root.getChild(i);
    let accesstime = Date(resultNode.time / 1000);
    dump("----> result: " + resultNode.uri + "   Date: " + accesstime.toLocaleString() + "\n");
  }
  do_check_eq(cc,0);
  root.containerOpen = false;
}
