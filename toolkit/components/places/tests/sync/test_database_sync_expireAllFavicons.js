










































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
var icons = Cc["@mozilla.org/browser/favicon-service;1"].
            getService(Ci.nsIFaviconService);

const TEST_URI = "http://test.com/";
const TEST_ICON_URI = "http://test.com/favicon.ico";
const TEST_NOSYNC_URI = "http://nosync.test.com/";
const TEST_NOSYNC_ICON_URI = "http://nosync.test.com/favicon.ico";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600;
const kSyncFinished = "places-sync-finished";

const kExpirationFinished = "places-favicons-expired";

var observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      os.removeObserver(this, kSyncFinished);

      
      hs.addVisit(uri(TEST_NOSYNC_URI), Date.now() * 1000, null,
                  hs.TRANSITION_TYPED, false, 0);
      
      icons.setFaviconUrlForPage(uri(TEST_NOSYNC_URI), uri(TEST_NOSYNC_ICON_URI));

      
      let stmt = dbConn.createStatement(
        "SELECT id FROM moz_places WHERE url = :url AND favicon_id NOT NULL");
      stmt.params["url"] = TEST_URI;
      do_check_true(stmt.executeStep());

      stmt.finalize();

      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places_temp WHERE url = :url AND favicon_id NOT NULL");
      stmt.params["url"] = TEST_NOSYNC_URI;
      do_check_true(stmt.executeStep());

      stmt.finalize();

      
      icons.expireAllFavicons();
    }
    else if (aTopic == kExpirationFinished) {
      os.removeObserver(this, kExpirationFinished);
      
      let stmt = dbConn.createStatement(
        "SELECT id FROM moz_favicons");
      do_check_false(stmt.executeStep());

      stmt.finalize();

      stmt = dbConn.createStatement(
        "SELECT id FROM moz_places_view WHERE favicon_id NOT NULL");
      do_check_false(stmt.executeStep());

      stmt.finalize();

      do_test_finished();
    }
  }
}
os.addObserver(observer, kSyncFinished, false);
os.addObserver(observer, kExpirationFinished, false);

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  let visitId = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                            hs.TRANSITION_TYPED, false, 0);
  
  icons.setFaviconUrlForPage(uri(TEST_URI), uri(TEST_ICON_URI));

  
  bs.insertBookmark(bs.toolbarFolder, uri(TEST_URI), bs.DEFAULT_INDEX, "visited");

  do_test_pending();
}
