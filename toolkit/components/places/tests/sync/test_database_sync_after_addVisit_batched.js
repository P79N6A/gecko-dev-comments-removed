







































  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

const TEST_URI = "http://test.com/";

const kSyncFinished = "places-sync-finished";


var bookmarksObserver = {
  _batching: false,
  onBeginUpdateBatch: function() {
    this._batching = true;
  },
  onEndUpdateBatch: function() {
    this._batching = false;
  }
}
bs.addObserver(bookmarksObserver, false);


var historyObserver = {
  onVisit: function(aURI, aVisitId, aTime, aSessionId, aReferringId,
                    aTransitionType, aAdded) {
    observer.visitId = aVisitId;
  }
}
hs.addObserver(historyObserver, false);

var observer = {
  visitId: -1,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      do_check_neq(this.visitId, -1);
      
      do_check_false(bookmarksObserver._batching);
      
      os.removeObserver(this, kSyncFinished);
      bs.removeObserver(bookmarksObserver);
      hs.removeObserver(historyObserver);
      
      new_test_visit_uri_event(this.visitId, TEST_URI, true, true);
    }
  }
}
os.addObserver(observer, kSyncFinished, false);

function run_test()
{
  
  let id = -1;
  bs.runInBatchMode({
    runBatched: function(aUserData)
    {
      id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);
      
      new_test_visit_uri_event(id, TEST_URI, false);
    }
  }, null);
  
  do_check_neq(id, -1);

  do_test_pending();
}
