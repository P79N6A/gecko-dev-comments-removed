



function run_test() {
  




  function forceBookmarkCaching(aItemId) {
    let parent = PlacesUtils.bookmarks.getFolderIdForItem(aItemId);
    PlacesUtils.bookmarks.getFolderIdForItem(parent);
  }

  let observer = {
    onBeginUpdateBatch: function () forceBookmarkCaching(itemId1),
    onEndUpdateBatch: function () forceBookmarkCaching(itemId1),
    onItemAdded: forceBookmarkCaching,
    onItemChanged: forceBookmarkCaching,
    onItemMoved: forceBookmarkCaching,
    onItemRemoved: function (id) {
      try {
        forceBookmarkCaching(id);
        do_throw("trying to fetch a removed bookmark should throw");
      } catch (ex) {}
    },
    onItemVisited: forceBookmarkCaching,
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver])
  };
  PlacesUtils.bookmarks.addObserver(observer, false);

  let folder1 = PlacesUtils.bookmarks
                           .createFolder(PlacesUtils.bookmarksMenuFolderId,
                                         "Folder1",
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
  let folder2 = PlacesUtils.bookmarks
                           .createFolder(folder1,
                                         "Folder2",
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
  let itemId = PlacesUtils.bookmarks
                           .insertBookmark(folder2,
                                           NetUtil.newURI("http://mozilla.org/"),
                                           PlacesUtils.bookmarks.DEFAULT_INDEX,
                                           "Mozilla");

  PlacesUtils.bookmarks.removeFolderChildren(folder1);

  
  do_check_eq(PlacesUtils.bookmarks.getItemTitle(folder1), "Folder1");
  try {
    PlacesUtils.bookmarks.getItemTitle(folder2);
    do_throw("trying to fetch a removed bookmark should throw");
  } catch (ex) {}

  PlacesUtils.bookmarks.removeObserver(observer, false);
}
