






































function run_test() {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var testURI = uri("wyciwyg://nodontjudgeabookbyitscover");

  var placeID = histsvc.addVisit(testURI,
                                 Date.now() * 1000,
                                 null,
                                 histsvc.TRANSITION_LINK,
                                 false, 
                                 0);
  do_check_false(placeID > 0);

  
  histsvc.QueryInterface(Ci.nsIGlobalHistory2);
  placeID = histsvc.addURI(testURI, false, false, null);
  do_check_false(placeID > 0);
}
