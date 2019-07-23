







































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
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
      new_test_visit_uri_event(id, TEST_URI, true, true);
    }
  }, (SYNC_INTERVAL * 1000) * 2, Ci.nsITimer.TYPE_ONE_SHOT);
  do_test_pending();
}
