









































do_test_pending();


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

var ghist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIGlobalHistory2);


var observer = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  visitCount: 0,
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) {
    this.visitCount++;
    dump("onVisit: " + aURI.spec + "\n");
    confirm_results();
  },
  onTitleChanged: function(aURI, aPageTitle) {},
  onBeforeDeleteURI: function(aURI) {},
  onDeleteURI: function(aURI) {},
  onClearHistory: function() {},
  onPageChanged: function(aURI, aWhat, aValue) {},
  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {},
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavHistoryObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

histsvc.addObserver(observer, false);


function run_test() {
  var now = Date.now();
  var testURI = uri("http://fez.com");
  ghist.addURI(testURI, false, true, null);
  ghist.addURI(testURI, false, true, testURI);
  
  do_timeout(3500, confirm_results);
}

function confirm_results() {
  var options = histsvc.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_VISIT;
  options.includeHidden = true;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, 1);
  root.containerOpen = false;
  do_test_finished();
}
