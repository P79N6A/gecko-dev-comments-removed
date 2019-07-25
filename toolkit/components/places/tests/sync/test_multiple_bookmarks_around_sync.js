













































var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var db = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsPIPlacesDatabase).
         DBConnection;
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
    observer.itemIds.push(aItemId);
  }
}
bs.addObserver(bookmarksObserver, false);

var observer = {
  itemIds: [],
  _placeId: -1,
  _runCount: 0,
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kSyncFinished) {
      if (++this._runCount == 1) {
        let itemId = this.itemIds[this._runCount - 1];
        
        do_check_neq(itemId, null);
        
        new_test_bookmark_uri_event(itemId, TEST_URI, true);

        
        let stmt = db.createStatement(
          "SELECT fk " +
          "FROM moz_bookmarks " +
          "WHERE id = ?"
        );
        stmt.bindInt64Parameter(0, itemId);
        do_check_true(stmt.executeStep());
        this._placeId = stmt.getInt64(0);
        stmt.finalize();
        stmt = null;
        
        do_check_true(this._placeId > 0);
      }
      else if (this._runCount == 2) {
        let itemId = this.itemIds[this._runCount - 1];
        
        do_check_neq(itemId, null);
        
        new_test_bookmark_uri_event(itemId, TEST_URI, true);

        
        stmt = db.createStatement(
          "SELECT * " +
          "FROM moz_bookmarks " +
          "WHERE id = ?1 " +
          "AND fk = ?2"
        );
        stmt.bindInt64Parameter(0, itemId);
        stmt.bindInt64Parameter(1, this._placeId);
        do_check_true(stmt.executeStep());
        stmt.finalize();
        stmt = null;

        
        os.removeObserver(this, kSyncFinished);
        bs.removeObserver(bookmarksObserver);
        
        do_test_finished();
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

  
  let id1 = bs.insertBookmark(bs.unfiledBookmarksFolder, uri(TEST_URI),
                              bs.DEFAULT_INDEX, "test");

  
  let id2 = bs.insertBookmark(bs.toolbarFolder, uri(TEST_URI),
                              bs.DEFAULT_INDEX, "test");
  do_check_neq(id1, id2);

  do_test_pending();
}
