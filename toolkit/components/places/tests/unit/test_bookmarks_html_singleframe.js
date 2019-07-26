








Cu.import("resource://gre/modules/BookmarkHTMLUtils.jsm");

function run_test() {
  do_test_pending();
  let bookmarksFile = do_get_file("bookmarks_html_singleframe.html");
  BookmarkHTMLUtils.importFromFile(bookmarksFile, true, after_import);
}

function after_import(success) {
  do_check_true(success);
  let root = PlacesUtils.getFolderContents(PlacesUtils.bookmarksMenuFolderId).root;
  do_check_eq(root.childCount, 1);
  let folder = root.getChild(0);
  PlacesUtils.asContainer(folder).containerOpen = true;
  do_check_eq(folder.title, "Subtitle");
  do_check_eq(folder.childCount, 1);
  let bookmark = folder.getChild(0);
  do_check_eq(bookmark.uri, "http://www.mozilla.org/");
  do_check_eq(bookmark.title, "Mozilla");
  folder.containerOpen = false;
  do_test_finished();
}