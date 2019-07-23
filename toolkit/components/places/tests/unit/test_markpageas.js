









































try {
  var gh = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get global history service\n");
} 


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

function add_uri_to_history(aURI) {
  gh.addURI(aURI,
            false, 
            true, 
            null); 
}

var gVisits = [{url: "http://www.mozilla.com/",
                transition: histsvc.TRANSITION_TYPED},
               {url: "http://www.google.com/", 
                transition: histsvc.TRANSITION_BOOKMARK},
               {url: "http://www.espn.com/",
                transition: histsvc.TRANSITION_LINK}];


function run_test() {
  for each (var visit in gVisits) {
    if (visit.transition == histsvc.TRANSITION_TYPED)
      gh.markPageAsTyped(uri(visit.url));
    else if (visit.transition == histsvc.TRANSITION_BOOKMARK)
      gh.markPageAsFollowedBookmark(uri(visit.url))
    else {
     
     
    }
    add_uri_to_history(uri(visit.url));
  }

  do_test_pending();
}


var observer = {
  _visitCount: 0,
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onVisit: function(aURI, aVisitID, aTime, aSessionID, 
                    aReferringID, aTransitionType, aAdded) {
    do_check_eq(aURI.spec, gVisits[this._visitCount].url);
    do_check_eq(aTransitionType, gVisits[this._visitCount].transition);
    this._visitCount++;

    if (this._visitCount == gVisits.length)
      do_test_finished();
  },
  onTitleChanged: function(aURI, aPageTitle) {
  },
  onBeforeDeleteURI: function(aURI) {
  },
  onDeleteURI: function(aURI) {
  },
  onClearHistory: function() {
  },
  onPageChanged: function(aURI, aWhat, aValue) {
  },
  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavHistoryObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

histsvc.addObserver(observer, false);
