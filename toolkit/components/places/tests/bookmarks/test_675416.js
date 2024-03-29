


function run_test() {
  




  function forceBookmarkCaching(aItemId) {
    PlacesUtils.bookmarks.getFolderIdForItem(aItemId);
  }

  let observer = {
    onBeginUpdateBatch: function() forceBookmarkCaching(itemId1),
    onEndUpdateBatch: function() forceBookmarkCaching(itemId1),
    onItemAdded: forceBookmarkCaching,
    onItemChanged: forceBookmarkCaching,
    onItemMoved: forceBookmarkCaching,
    onItemRemoved: function(id) {
      try {
        forceBookmarkCaching(id);
        do_throw("trying to fetch a removed bookmark should throw");
      } catch (ex) {}
    },
    onItemVisited: forceBookmarkCaching,
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver])
  };
  PlacesUtils.bookmarks.addObserver(observer, false);

  let folderId1 = PlacesUtils.bookmarks
                             .createFolder(PlacesUtils.bookmarksMenuFolderId,
                                           "Bookmarks",
                                           PlacesUtils.bookmarks.DEFAULT_INDEX);
  let itemId1 = PlacesUtils.bookmarks
                           .insertBookmark(folderId1,
                                           NetUtil.newURI("http:/www.wired.com/wiredscience"),
                                           PlacesUtils.bookmarks.DEFAULT_INDEX,
                                           "Wired Science");

  PlacesUtils.bookmarks.removeItem(folderId1);

  let folderId2 = PlacesUtils.bookmarks
                             .createFolder(PlacesUtils.bookmarksMenuFolderId,
                                           "Science",
                                           PlacesUtils.bookmarks.DEFAULT_INDEX);
  let folderId3 = PlacesUtils.bookmarks
                             .createFolder(folderId2,
                                           "Blogs",
                                           PlacesUtils.bookmarks.DEFAULT_INDEX);
  
  do_check_eq(PlacesUtils.bookmarks.getItemTitle(folderId3), "Blogs");
  do_check_eq(PlacesUtils.bookmarks.getItemTitle(folderId2), "Science");

  PlacesUtils.bookmarks.removeObserver(observer, false);
}
