


function run_test() {
  do_test_pending();
  
  const TEST_URI = NetUtil.newURI("http://moz.org/")
  let observer = {
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsINavBookmarkObserver,
    ]),

    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onItemAdded: function (aItemId, aParentId, aIndex, aItemType, aURI) {
      do_check_true(aURI.equals(TEST_URI));
      PlacesUtils.removeLazyBookmarkObserver(this);
      do_test_finished();
    },
    onItemRemoved: function () {},
    onItemChanged: function () {},
    onItemVisited: function () {},
    onItemMoved: function () {},
  };

  
  PlacesUtils.addLazyBookmarkObserver(observer);
  PlacesUtils.removeLazyBookmarkObserver(observer);

  
  PlacesUtils.addLazyBookmarkObserver(observer);

  
  
  PlacesUtils.bookmarks;
  PlacesUtils.addLazyBookmarkObserver(observer);
  PlacesUtils.removeLazyBookmarkObserver(observer);
  try {
    PlacesUtils.bookmarks.removeObserver(observer);
    do_throw("Trying to remove a nonexisting observer should throw!");
  } catch (ex) {}

  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       TEST_URI,
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "Bookmark title");
}
