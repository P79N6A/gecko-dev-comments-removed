
















































function run_test() {
    const IDX = PlacesUtils.bookmarks.DEFAULT_INDEX;
    var folderId = 
      PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId, "", IDX);

    var queryId =
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                           uri("place:folder=" + folderId), IDX, "");
    
    var root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId, false, true).root;

    var oldCount = root.childCount;

    PlacesUtils.bookmarks.removeItem(queryId);

    do_check_eq(root.childCount, oldCount-1);

    root.containerOpen = false;
}
