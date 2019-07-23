






































 




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

var syncObserver = {
  _numSyncs: 0,
  finalSync: false,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      if (++this._numSyncs > 1 || !this.finalSync)
        do_throw("We synced too many times: " + this._numSyncs);

      
      os.removeObserver(this, kSyncFinished);
      bs.removeObserver(bookmarksObserver, false);

      do_test_finished();
    }
  }
}
os.addObserver(syncObserver, kSyncFinished, false);

var bookmarksObserver = {
  onItemAdded: function(aItemId, aNewParent, aNewIndex) {
    if (bs.getItemType(aItemId) == bs.TYPE_BOOKMARK)
      syncObserver.finalSync = true;
  }
}
bs.addObserver(bookmarksObserver, false);

function run_test()
{
  
  do_load_module("../unit/nsDynamicContainerServiceSample.js");

  
  prefs.setIntPref(kSyncPrefName, SYNC_INTERVAL);

  
  bs.createFolder(bs.toolbarFolder, "folder", bs.DEFAULT_INDEX);

  
  bs.createDynamicContainer(bs.toolbarFolder, "dynamic",
                                "@mozilla.org/browser/remote-container-sample;1",
                                bs.DEFAULT_INDEX);

  
  bs.insertSeparator(bs.toolbarFolder, bs.DEFAULT_INDEX);

  
  bs.insertBookmark(bs.toolbarFolder, uri(TEST_URI), bs.DEFAULT_INDEX, "bookmark");

  do_test_pending();
}
