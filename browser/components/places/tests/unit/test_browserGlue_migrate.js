











































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "bs",
                                   "@mozilla.org/browser/nav-bookmarks-service;1",
                                   "nsINavBookmarksService");
XPCOMUtils.defineLazyServiceGetter(this, "anno",
                                   "@mozilla.org/browser/annotation-service;1",
                                   "nsIAnnotationService");

let bookmarksObserver = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
    do_check_neq(itemId, -1);
    if (anno.itemHasAnnotation(itemId, "Places/SmartBookmark"))
      continue_test();
  },
  onItemAdded: function() {},
  onBeforeItemRemoved: function(id) {},
  onItemRemoved: function(id, folder, index, itemType) {},
  onItemChanged: function() {},
  onItemVisited: function(id, visitID, time) {},
  onItemMoved: function() {},
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver])
};

const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";

function run_test() {
  do_test_pending();

  
  
  create_bookmarks_html("bookmarks.glue.html");

  
  let db = gProfD.clone();
  db.append("places.sqlite");
  if (db.exists()) {
    db.remove(false);
    do_check_false(db.exists());
  }

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  
  
  do_check_eq(hs.databaseStatus, hs.DATABASE_STATUS_CREATE);

  
  
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://mozilla.org/"),
                    bs.DEFAULT_INDEX, "migrated");

  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].
           getService(Ci.nsIBrowserGlue);

  
  
  bs.addObserver(bookmarksObserver, false);
}

function continue_test() {
  
  let itemId = bs.getIdForItemAt(bs.bookmarksMenuFolder, SMART_BOOKMARKS_ON_MENU);
  do_check_eq(bs.getItemTitle(itemId), "migrated");

  
  do_check_eq(bs.getIdForItemAt(bs.bookmarksMenuFolder, SMART_BOOKMARKS_ON_MENU + 1), -1);
  do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_MENU), -1);

  remove_bookmarks_html();

  do_test_finished();
}
