








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

  
  
  do_check_eq(PlacesUtils.history.databaseStatus,
              PlacesUtils.history.DATABASE_STATUS_CREATE);

  
  
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.bookmarksMenuFolder, uri("http://mozilla.org/"),
                    PlacesUtils.bookmarks.DEFAULT_INDEX, "migrated");

  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].
           getService(Ci.nsIBrowserGlue);

  let bookmarksObserver = {
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {
      
      let itemId =
        PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
      do_check_neq(itemId, -1);
      if (PlacesUtils.annotations
                     .itemHasAnnotation(itemId, "Places/SmartBookmark")) {
        do_execute_soon(onSmartBookmarksCreation);
      }
    },
    onItemAdded: function() {},
    onBeforeItemRemoved: function(id) {},
    onItemRemoved: function(id, folder, index, itemType) {},
    onItemChanged: function() {},
    onItemVisited: function(id, visitID, time) {},
    onItemMoved: function() {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver])
  };
  
  
  PlacesUtils.bookmarks.addObserver(bookmarksObserver, false);
}

function onSmartBookmarksCreation() {
  
  let itemId =
    PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId,
                                         SMART_BOOKMARKS_ON_MENU);
  do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "migrated");

  
  itemId =
    PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId,
                                         SMART_BOOKMARKS_ON_MENU + 1)
  do_check_eq(itemId, -1);
  itemId =
    PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId,
                                         SMART_BOOKMARKS_ON_MENU)
  do_check_eq(itemId, -1);

  remove_bookmarks_html();

  do_test_finished();
}
