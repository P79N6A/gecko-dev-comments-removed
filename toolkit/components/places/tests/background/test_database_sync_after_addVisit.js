






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";
const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 1;

function run_test()
{
  
  let prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefService).
              getBranch("places.");
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  let id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);

  
  
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback({
    notify: function(aTimer)
    {
      PlacesBackground.dispatch(new_test_visit_uri_event(id, TEST_URI, true, true),
                                Ci.nsIEventTarget.DISPATCH_NORMAL);
    }
  }, (SYNC_INTERVAL * 1000) * 2, Ci.nsITimer.TYPE_ONE_SHOT);
  do_test_pending();
}
