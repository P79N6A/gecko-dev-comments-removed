





































Components.utils.import("resource://gre/modules/utils.js");
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

  
  try {
    PlacesUtils.bookmarks.removeFolder(bookmarkId);
    do_throw("no exception when removing a bookmark via removeFolder()!");
  } catch(ex) {}
  do_check_true(PlacesUtils.bookmarks.isBookmarked(bookmarkURI));

  
  PlacesUtils.bookmarks.removeItem(folderId);
  do_check_eq(PlacesUtils.bookmarks.getBookmarkIdsForURI(bookmarkURI, {}).length, 0);
  do_check_false(PlacesUtils.bookmarks.isBookmarked(bookmarkURI));
  do_check_eq(PlacesUtils.bookmarks.getItemIndex(bookmarkId), -1);
}
