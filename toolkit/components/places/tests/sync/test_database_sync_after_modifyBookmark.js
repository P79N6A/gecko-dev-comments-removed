







































var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService).
            getBranch("places.");

const TEST_URI = "http://test.com/";
const MODIFIED_URI = "http://test.com/index.html";

const SYNC_INTERVAL = 600; 
const kSyncPrefName = "syncDBTableIntervalInSecs";
const kSyncFinished = "places-sync-finished";


var bookmarksObserver = {
  onItemAdded: function(aItemId, aNewParent, aNewIndex, aItemType) {
    observer.itemId = aItemId;
  },
  onItemChanged: function(aItemId, aProperty, aNewValue, aLastModified,
                          aItemType) {
    if (aProperty == "uri")
      do_check_eq(observer.itemId, aItemId);
  }
}
bs.addObserver(bookmarksObserver, false);

var observer = {
  itemId: -1,
  _runCount: 0,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      
      do_check_neq(this.itemId, -1);
      if (++this._runCount == 1) {
        
        
        new_test_bookmark_uri_event(this.itemId, TEST_URI, true);
        
        bs.changeBookmarkURI(this.itemId, uri(MODIFIED_URI));
      }
      else if (this._runCount == 2) {
        
        
        os.removeObserver(this, kSyncFinished);
        bs.removeObserver(bookmarksObserver);
        
        new_test_bookmark_uri_event(this.itemId, MODIFIED_URI, true, true);
      }
      else
        do_throw("Too many places sync calls");
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
