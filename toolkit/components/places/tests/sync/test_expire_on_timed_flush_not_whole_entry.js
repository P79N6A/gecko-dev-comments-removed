






































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = Cc["@mozilla.org/browser/global-history;2"].
         getService(Ci.nsIBrowserHistory);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const TEST_URI = "http://test.com/";

const kSyncPrefName = "places.syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 1;
const kSyncFinished = "places-sync-finished";

const kExpireDaysPrefName = "browser.history_expire_days";
const EXPIRE_DAYS = 1;

var observer = {
  visitId1: -1,
  visitId2: -1,
  notificationReceived: false,
  _syncCount: 0,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      
      if (this._syncCount++ == 0)
        return;

      
      do_check_neq(this.visitId1, -1);
      do_check_neq(this.visitId2, -1);

      
      do_check_true(this.notificationReceived);

      
      os.removeObserver(this, kSyncFinished);
      
      new_test_visit_uri_event(this.visitId1, TEST_URI, false);
      new_test_visit_uri_event(this.visitId2, TEST_URI, true, true);
    }
  }
}
os.addObserver(observer, kSyncFinished, false);


var historyObserver = {
  visitTime: -1,
  onPageExpired: function(aURI, aVisitTime, aWholeEntry)
  {
    do_check_true(aURI.equals(uri(TEST_URI)));

    
    do_check_eq(this.visitTime, aVisitTime);

    
    do_check_false(aWholeEntry);

    
    do_check_false(observer.notificationReceived);
    observer.notificationReceived = true;
    hs.removeObserver(this, false);
  }
}
hs.addObserver(historyObserver, false);

function run_test()
{
  
  prefs.setIntPref(kExpireDaysPrefName, EXPIRE_DAYS);
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  do_check_false(bh.isVisited(uri(TEST_URI)));

  
  let date = Date.now() - 2 * 24 * 60 * 60 * 1000; 
  observer.visitId1 = hs.addVisit(uri(TEST_URI), date * 1000, null,
                                  hs.TRANSITION_TYPED, false, 0);
  historyObserver.visitTime = date * 1000;

  
  observer.visitId2 = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                                  hs.TRANSITION_TYPED, false, 0);

  do_test_pending();
}
