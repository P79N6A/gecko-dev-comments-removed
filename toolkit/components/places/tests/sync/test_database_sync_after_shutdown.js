







































var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

const TEST_URI = "http://test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600; 
const kSyncFinished = "places-sync-finished";

var historyObserver = {
  onVisit: function(aURI, aVisitId, aTime, aSessionId, aReferringId,
                    aTransitionType, aAdded) {
    observer.visitId = aVisitId;
    hs.removeObserver(this);
  }
}
hs.addObserver(historyObserver, false);

var observer = {
  visitId: -1,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      os.removeObserver(this, kSyncFinished);

      
      do_check_neq(this.visitId, -1);
      
      new_test_visit_uri_event(this.visitId, TEST_URI, true, true);
    }
  }
}
os.addObserver(observer, kSyncFinished, false);

function run_test()
{
  
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);

  
  shutdownPlaces();

  
  do_test_pending();
}
