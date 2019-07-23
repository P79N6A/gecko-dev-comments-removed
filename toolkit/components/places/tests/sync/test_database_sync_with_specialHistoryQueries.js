






































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const TEST_URI = "http://test.com/";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600;
const kSyncFinished = "places-sync-finished";

var observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

      
      
      hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                  hs.TRANSITION_TYPED, false, 1);
  
      
      var options = hs.getNewQueryOptions();
      options.maxResults = 10;
      options.resultType = options.RESULTS_AS_URI;
      options.sortingMode = options.SORT_BY_DATE_DESCENDING;
      var query = hs.getNewQuery();
      var result = hs.executeQuery(query, options);
      var root = result.root;
      root.containerOpen = true;
      do_check_eq(root.childCount, 1);
      root.containerOpen = false;

      
      options = hs.getNewQueryOptions();
      options.maxResults = 10;
      options.resultType = options.RESULTS_AS_URI;
      options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
      query = hs.getNewQuery();
      result = hs.executeQuery(query, options);
      root = result.root;
      root.containerOpen = true;
      do_check_eq(root.childCount, 1);
      root.containerOpen = false;

      
      options = hs.getNewQueryOptions();
      query = hs.getNewQuery();
      result = hs.executeQuery(query, options);
      root = result.root;
      root.containerOpen = true;
      do_check_eq(root.childCount, 1);
      root.containerOpen = false;

      os.removeObserver(this, kSyncFinished);
      finish_test();
    }
  }
}
os.addObserver(observer, kSyncFinished, false);

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, 1);

  
  let visitId = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                            hs.TRANSITION_TYPED, false, 0);
  do_test_pending();
}
