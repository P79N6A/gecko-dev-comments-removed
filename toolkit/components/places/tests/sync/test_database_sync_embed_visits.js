






































 




var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var dbConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const TEST_URI = "http://test.com/";
const EMBED_URI = "http://embed.test.com/";
const PLACE_URI = "place:test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 1;
const kSyncFinished = "places-sync-finished";

var transitions = [ hs.TRANSITION_LINK,
                    hs.TRANSITION_TYPED,
                    hs.TRANSITION_BOOKMARK,
                    hs.TRANSITION_EMBED,
                    hs.TRANSITION_REDIRECT_PERMANENT,
                    hs.TRANSITION_REDIRECT_TEMPORARY,
                    hs.TRANSITION_DOWNLOAD ];

var observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished && this.visitId != -1) {
      
      os.removeObserver(this, kSyncFinished);

      
      var stmt = dbConn.createStatement(
        "SELECT id FROM moz_places WHERE url = :url");
      stmt.params["url"] = TEST_URI;
      do_check_true(stmt.executeStep());
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places_temp WHERE url = :url");
      stmt.params["url"] = TEST_URI;
      do_check_false(stmt.executeStep());
      stmt.finalize();

      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places WHERE url = :url");
      stmt.params["url"] = EMBED_URI;
      do_check_false(stmt.executeStep());
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places_temp WHERE url = :url");
      stmt.params["url"] = EMBED_URI;
      do_check_true(stmt.executeStep());
      stmt.finalize();

      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places WHERE url = :url");
      stmt.params["url"] = PLACE_URI;
      do_check_true(stmt.executeStep());
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places_temp WHERE url = :url");
      stmt.params["url"] = PLACE_URI;
      do_check_false(stmt.executeStep());
      stmt.finalize();

      
      stmt = dbConn.createStatement(
        "SELECT count(*) FROM moz_historyvisits h WHERE visit_type <> :t_embed");
      stmt.params["t_embed"] = hs.TRANSITION_EMBED;
      do_check_true(stmt.executeStep());
      do_check_eq(stmt.getInt32(0), (transitions.length - 1) * 2);
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_historyvisits h WHERE visit_type = :t_embed");
      stmt.params["t_embed"] = hs.TRANSITION_EMBED;
      do_check_false(stmt.executeStep());
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_historyvisits_temp h WHERE visit_type = :t_embed");
      stmt.params["t_embed"] = hs.TRANSITION_EMBED;
      do_check_true(stmt.executeStep());
      stmt.finalize();
      stmt = dbConn.createStatement(
        "SELECT id FROM moz_historyvisits_temp h WHERE visit_type <> :t_embed");
      stmt.params["t_embed"] = hs.TRANSITION_EMBED;
      do_check_false(stmt.executeStep());
      stmt.finalize();

      do_test_finished();
    }
  }
}

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  
  
  transitions.forEach(function addVisit(aTransition) {
                        hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                                    aTransition, false, 0);
                      });

  
  
  
  hs.addVisit(uri(EMBED_URI), Date.now() * 1000, null,
              hs.TRANSITION_EMBED, false, 0);

  
  
  
  transitions.forEach(function addVisit(aTransition) {
                        hs.addVisit(uri(PLACE_URI), Date.now() * 1000, null,
                                    aTransition, false, 0);
                      });

  os.addObserver(observer, kSyncFinished, false);

  do_test_pending();
}
