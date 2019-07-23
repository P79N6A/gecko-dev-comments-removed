





































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
}

function add_visit(aURI, aDate, aReferrer, aType, isRedirect, aSessionID) {
  var placeID = histsvc.addVisit(aURI,
                                 aDate,
                                 aReferrer,
                                 aType,
                                 isRedirect,
                                 aSessionID);
  do_check_true(placeID > 0);
  return placeID;
}

function modHistoryTypes(val){
  switch(val % 7) {
    case 0:
    case 1:
      return histsvc.TRANSITION_LINK;
    case 2:
      return histsvc.TRANSITION_TYPED;
    case 3:
      return histsvc.TRANSITION_BOOKMARK;
    case 4:
      return histsvc.TRANSITION_EMBED;
    case 5:
      return histsvc.TRANSITION_REDIRECT_PERMANENT;
    case 6:
      return histsvc.TRANSITION_REDIRECT_TEMPORARY;
    case 7:
      return histsvc.TRANSITION_DOWNLOAD;
  }
  return histsvc.TRANSITION_TYPED;
}





function buildTestDatabase() {
  
  
  var testURI = uri("http://www.foo.com");
  var testAnnoName = "moz-test-places/testing123";
  var testAnnoVal = "test";

  for (var i=0; i < 12; ++i)
    add_visit(testURI,
              today,
              null,
              modHistoryTypes(i), 
              false,
              0);

  testURI = uri("http://foo.com/youdontseeme.html");
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, 0);
  for (var i=0; i < 12; ++i)
    add_visit(testURI,
              today,
              null,
              modHistoryTypes(i), 
              false,
              0);
}










function run_test() {
  buildTestDatabase();
  var query = histsvc.getNewQuery();
  query.annotation = "moz-test-places/testing123";
  query.beginTime = daybefore * 1000;
  query.beginTimeReference = histsvc.TIME_RELATIVE_NOW;
  query.endTime = today * 1000;
  query.endTimeReference = histsvc.TIME_RELATIVE_NOW;
  query.minVisits = 2;
  query.maxVisits = 10;

  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  dump("----> cc is: " + cc + "\n");
  for(var i=0; i < root.childCount; ++i) {
    var resultNode = root.getChild(i);
    var accesstime = Date(resultNode.time);
    dump("----> result: " + resultNode.uri + "   Date: " + accesstime.toLocaleString() + "\n");
  }
  do_check_eq(cc,0);
}
