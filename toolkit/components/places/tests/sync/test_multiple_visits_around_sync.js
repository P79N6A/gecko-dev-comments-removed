













































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var db = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsPIPlacesDatabase).
         DBConnection;
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const TEST_URI = "http://test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 1;
const kSyncFinished = "places-sync-finished";

var observer = {
  visitId: -1,
  _runCount: 0,
  _placeId: -1,
  _lastVisitId: -1,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished && this.visitId != -1) {
      
      new_test_visit_uri_event(this.visitId, TEST_URI, true);

      
      
      do_check_eq(this.visitId, visitIds[this._runCount]);

      if (++this._runCount == 1) {
        
        let stmt = db.createStatement(
          "SELECT place_id " +
          "FROM moz_historyvisits " +
          "WHERE id = ?1"
        );
        stmt.bindInt64Parameter(0, this.visitId);
        do_check_true(stmt.executeStep());
        this._placeId = stmt.getInt64(0);
        this._lastVisitId = this.visitId;
        stmt.finalize();
        stmt = null;

        
        this.visitId = -1;
        continue_test();
      }
      else if (this._runCount == 2) {
        do_check_neq(this.visitId, this._lastVisitId);
        
        let stmt = db.createStatement(
          "SELECT place_id " +
          "FROM moz_historyvisits " +
          "WHERE id = ?1"
        );
        stmt.bindInt64Parameter(0, this.visitId);
        do_check_true(stmt.executeStep());
        do_check_eq(this._placeId, stmt.getInt64(0));
        stmt.finalize();
        stmt = null;

        
        os.removeObserver(this, kSyncFinished);
        hs.removeObserver(historyObserver, false);
        do_test_finished();
      }
      else
        do_throw("bad runCount!");
    }
  }
}
os.addObserver(observer, kSyncFinished, false);


var historyObserver = {
  onVisit: function(aURI, aVisitId, aTime, aSessionId, aReferringId,
                    aTransitionType, aAdded) {
    do_check_true(aVisitId > 0);
    observer.visitId = aVisitId;
  }
}
hs.addObserver(historyObserver, false);


var visitIds = [];

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  visitIds[0] = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                            hs.TRANSITION_TYPED, false, 0);

  do_test_pending();
}

function continue_test()
{
  
  visitIds[1] = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                            hs.TRANSITION_TYPED, false, 0);
}
