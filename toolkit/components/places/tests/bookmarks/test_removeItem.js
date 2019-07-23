





































var tests = [];


const DEFAULT_INDEX = PlacesUtils.bookmarks.DEFAULT_INDEX;

function run_test() {
  
  var folderId =
    PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId,
                                       "", DEFAULT_INDEX);

  
  var bookmarkURI = uri("http://iasdjkf");
  do_check_false(PlacesUtils.bookmarks.isBookmarked(bookmarkURI));
  var bookmarkId = PlacesUtils.bookmarks.insertBookmark(folderId, bookmarkURI,
                                                        DEFAULT_INDEX, "");
  do_check_eq(PlacesUtils.bookmarks.getItemTitle(bookmarkId), "");

  
  PlacesUtils.bookmarks.removeItem(folderId);
  do_check_eq(PlacesUtils.bookmarks.getBookmarkIdsForURI(bookmarkURI).length, 0);
  do_check_false(PlacesUtils.bookmarks.isBookmarked(bookmarkURI));
  do_check_eq(PlacesUtils.bookmarks.getItemIndex(bookmarkId), -1);
}
