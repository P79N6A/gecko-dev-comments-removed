


function run_test() {
  PlacesUtils.bookmarks.insertBookmark(
    PlacesUtils.unfiledBookmarksFolderId, NetUtil.newURI("http://1.moz.org/"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, "Bookmark 1"
  );
  let id = PlacesUtils.bookmarks.insertBookmark(
    PlacesUtils.unfiledBookmarksFolderId, NetUtil.newURI("place:folder=1234"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, "Shortcut"
  );
  PlacesUtils.bookmarks.insertBookmark(
    PlacesUtils.unfiledBookmarksFolderId, NetUtil.newURI("http://2.moz.org/"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, "Bookmark 2"
  );

  
  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.unfiledBookmarksFolderId], 1);
  let options = PlacesUtils.history.getNewQueryOptions();
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 3);
  let shortcut = root.getChild(1);
  do_check_eq(shortcut.uri, "place:folder=1234");
  PlacesUtils.asContainer(shortcut);
  shortcut.containerOpen = true;
  do_check_eq(shortcut.childCount, 0);
  shortcut.containerOpen = false;
  
  PlacesUtils.bookmarks.removeItem(id);
  do_check_eq(root.childCount, 2);
  root.containerOpen = false;

  
  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([1234], 1);
  let options = PlacesUtils.history.getNewQueryOptions();
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 0);
  root.containerOpen = false;
}
