







































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
  onItemAdded: function(aItemId, aNewParent, aNewIndex) {
    observer.itemId = aItemId;
  }
}
bs.addObserver(bookmarksObserver, false);

var observer = {
  itemId: -1,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      do_check_neq(this.itemId, -1);
      
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

  
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri(TEST_URI),
                    bs.DEFAULT_INDEX, "test");

  do_test_pending();
}
