






































var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

const TEST_URI = "http://test.com/";

const PREF_SYNC_INTERVAL = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600; 
const TOPIC_SYNC_FINISHED = "places-sync-finished";


const POLLING_TIMEOUT_MS = 100;
const POLLING_MAX_PASSES = 20;

var historyObserver = {
  visitId: -1,
  cleared: false,
  onVisit: function(aURI, aVisitId, aTime, aSessionId, aReferringId,
                    aTransitionType, aAdded) {
    this.visitId = aVisitId;
  },
  onClearHistory: function() {
    
    do_check_eq(0, bh.count);
    this.cleared = true;
    hs.removeObserver(this);
  }
}
hs.addObserver(historyObserver, false);

var observer = {
  _runCount: 0,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == TOPIC_SYNC_FINISHED) {
      if (++this._runCount == 1) {
        
        
        bh.removeAllPages();
        
        shutdownPlaces();
        return;
      }

      
      os.removeObserver(this, TOPIC_SYNC_FINISHED);

      
      do_check_neq(historyObserver.visitId, -1);
      
      do_check_true(historyObserver.cleared);

      
      
      do_timeout(POLLING_TIMEOUT_MS, check_results);
    }
  }
}
os.addObserver(observer, TOPIC_SYNC_FINISHED, false);

var gPasses = 0;
function check_results() {
    if (++gPasses >= POLLING_MAX_PASSES) {
      do_throw("Maximum time elapsdes waiting for Places database connection to close");
      do_test_finished();
    }

    if (hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection.connectionReady) {
      do_timeout(POLLING_TIMEOUT_MS, check_results);
      return;
    }

    let dbConn = DBConn();
    do_check_neq(dbConn, null);
    do_check_true(dbConn.connectionReady);

    
    
    
    let stmt = dbConn.createStatement(
      "SELECT id FROM moz_places WHERE frecency > 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    stmt = DBConn().createStatement(
      "SELECT h.id FROM moz_places h WHERE h.frecency = -2 " +
        "AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
    do_check_true(stmt.executeStep());
    stmt.finalize();

    
    stmt = DBConn().createStatement(
      "SELECT id FROM moz_places WHERE visit_count <> 0 LIMIT 1");
    do_check_false(stmt.executeStep());
    stmt.finalize();

    dbConn.close();
    do_check_false(dbConn.connectionReady);

    do_test_finished();
}

function run_test()
{
  do_test_pending();

  
  
  prefs.setIntPref(PREF_SYNC_INTERVAL, SYNC_INTERVAL);

  
  hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri(TEST_URI),
                    bs.DEFAULT_INDEX, "bookmark");
  hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
              hs.TRANSITION_TYPED, false, 0);
}
