







































var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");

const TEST_URI = "http://test.com/";

const SYNC_INTERVAL = 600; 
const kSyncPrefName = "syncDBTableIntervalInSecs";
const kSyncFinished = "places-sync-finished";


var bookmarksObserver = {
  _batching: false,
  onBeginUpdateBatch: function() {
    this._batching = true;
  },
  onEndUpdateBatch: function() {
    this._batching = false;
  },
  onItemAdded: function(aItemId, aNewParent, aNewIndex) {
    observer.itemId = aItemId;
  }
}
bs.addObserver(bookmarksObserver, false);

var observer = {
  itemId: -1,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      dump(this.itemId);
      
      do_check_neq(this.itemId, -1);
      
      do_check_false(bookmarksObserver._batching);
      
      os.removeObserver(this, kSyncFinished);
      bs.removeObserver(bookmarksObserver);
      
      new_test_bookmark_uri_event(this.itemId, TEST_URI, true, true);
    }
  }
}
os.addObserver(observer, kSyncFinished, false);

function run_test()
{
  
  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  let id = -1;
  bs.runInBatchMode({
    runBatched: function(aUserData)
    {
      id = bs.insertBookmark(bs.unfiledBookmarksFolder, uri(TEST_URI),
                             bs.DEFAULT_INDEX, "test");
      
      new_test_bookmark_uri_event(id, TEST_URI, false);
    }
  }, null);
  
  do_check_neq(id, -1);

  do_test_pending();
}
