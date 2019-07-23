






































var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var mDBConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

const TEST_URI = "http://test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600; 
const kSyncFinished = "places-sync-finished";
const kQuitApplication = "quit-application";

var historyObserver = {
  onVisit: function(aURI, aVisitId, aTime, aSessionId, aReferringId,
                    aTransitionType, aAdded) {
    observer.visitId = aVisitId;
  },
  onClearHistory: function() {
    
    do_check_eq(0, bh.count);
  }
}
hs.addObserver(historyObserver, false);

var observer = {
  visitId: -1,
  _runCount: 0,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      
      
      
      if (++this._runCount < 2)
        return;
      
      do_check_neq(this.visitId, -1);
      
      os.removeObserver(this, kSyncFinished);
      hs.removeObserver(historyObserver);
      
      
      
      
      dump_table("moz_places_temp");
      dump_table("moz_places");
      stmt = mDBConn.createStatement(
        "SELECT id FROM moz_places WHERE frecency > 0 LIMIT 1");
      do_check_false(stmt.executeStep());
      stmt.finalize();

      stmt = mDBConn.createStatement(
        "SELECT h.id FROM moz_places h WHERE h.frecency = -2 " +
          "AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
      do_check_true(stmt.executeStep());
      stmt.finalize();

      
      stmt = mDBConn.createStatement(
        "SELECT id FROM moz_places WHERE visit_count <> 0 LIMIT 1");
      do_check_false(stmt.executeStep());
      stmt.finalize();

      finish_test();
    }
    else if (aTopic == kQuitApplication) {
      
      bh.removeAllPages();
    }
  }
}
os.addObserver(observer, kSyncFinished, false);
os.addObserver(observer, kQuitApplication, false);

function run_test()
{
  
  
  let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  while (tm.mainThread.hasPendingEvents())
    tm.mainThread.processNextEvent(false);

  
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri(TEST_URI),
                    bs.DEFAULT_INDEX, "bookmark");
  hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);

  
  os.notifyObservers(null, kQuitApplication, null);

  do_test_pending();
}
