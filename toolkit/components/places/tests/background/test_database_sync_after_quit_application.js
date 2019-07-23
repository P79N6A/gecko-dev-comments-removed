






































Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";
const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600; 

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

  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.notifyObservers(null, "quit-application", null);

  
  
  new_test_visit_uri_event(id, TEST_URI, true, false).run();
}
