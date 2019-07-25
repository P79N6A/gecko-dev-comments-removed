













































 
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const kSyncPrefName = "syncDBTableIntervalInSecs";
const SYNC_INTERVAL = 600;
const kSyncFinished = "places-sync-finished";

var observer = {
  _runCount: 0,
  one: null,
  two: null,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      if (++this._runCount < 2)
        return;

      os.removeObserver(this, kSyncFinished);

      
      do_check_true(this.one < this.two);
      do_check_eq(bs.getItemIndex(this.one), 0);
      do_check_eq(bs.getItemIndex(this.two), 1);

      check_results();

      
      hs.addVisit(uri("http://2.mozilla.org/"), Date.now() * 1000, null,
                  hs.TRANSITION_TYPED, false, 0);

      
      check_results();

      
      bs.removeFolderChildren(bs.toolbarFolder);
      do_test_finished();
    }
  }
}
os.addObserver(observer, kSyncFinished, false);

function check_results() {
  
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  query.searchTerms = "mozilla";
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 2);
  do_check_eq(root.getChild(0).title, "mozilla 1");
  do_check_eq(root.getChild(1).title, "mozilla 2");
  root.containerOpen = false;
}

function run_test()
{
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  observer.one = bs.insertBookmark(bs.toolbarFolder,
                                   uri("http://1.mozilla.org/"),
                                   bs.DEFAULT_INDEX, "mozilla 1");
  observer.two = bs.insertBookmark(bs.toolbarFolder,
                                   uri("http://2.mozilla.org/"),
                                   bs.DEFAULT_INDEX, "mozilla 2");

  do_test_pending();
}
