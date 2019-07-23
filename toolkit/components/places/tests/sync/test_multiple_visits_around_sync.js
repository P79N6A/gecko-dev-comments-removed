













































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var db = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsPIPlacesDatabase).
         DBConnection;
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");

const TEST_URI = "http://test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 1;

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  let id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);

  
  
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback({
    notify: function(aTimer)
    {
      new_test_visit_uri_event(id, TEST_URI, true);

      
      let db = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
      let stmt = db.createStatement(
        "SELECT place_id " +
        "FROM moz_historyvisits " +
        "WHERE id = ?"
      );
      stmt.bindInt64Parameter(0, id);
      do_check_true(stmt.executeStep());
      continue_test(id, stmt.getInt64(0));
      stmt.finalize();
      stmt = null;
    }
  }, (SYNC_INTERVAL * 1000) * 2, Ci.nsITimer.TYPE_ONE_SHOT);
  do_test_pending();
}

function continue_test(aLastVisitId, aPlaceId)
{
  
  let id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);
  do_check_neq(aLastVisitId, id);

  
  
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback({
    notify: function(aTimer)
    {
      new_test_visit_uri_event(id, TEST_URI, true);

      
      let db = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
      let stmt = db.createStatement(
        "SELECT * " +
        "FROM moz_historyvisits " +
        "WHERE id = ?1 " +
        "AND place_id = ?2"
      );
      stmt.bindInt64Parameter(0, id);
      stmt.bindInt64Parameter(1, aPlaceId);
      do_check_true(stmt.executeStep());
      stmt.finalize();
      stmt = null;

      finish_test();
    }
  }, (SYNC_INTERVAL * 1000) * 2, Ci.nsITimer.TYPE_ONE_SHOT);
}
