








add_task(function* () {
  let folder = PlacesUtils.bookmarks
                          .createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                        "Folder",
                                        PlacesUtils.bookmarks.DEFAULT_INDEX);
  let bookmark = PlacesUtils.bookmarks
                            .insertBookmark(folder, NetUtil.newURI("http://example.com/"),
                                            PlacesUtils.bookmarks.DEFAULT_INDEX,
                                            "Bookmark");

  let library = yield promiseLibrary("AllBookmarks");
  registerCleanupFunction(function () {
    library.close();
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  });

  
  library.PlacesOrganizer.selectLeftPaneQuery("UnfiledBookmarks");
  ok(!library.PlacesOrganizer._places.selectedNode.containerOpen,
     "Unfiled container is closed");

  let folderNode = library.ContentTree.view.view.nodeForTreeIndex(0);
  is(folderNode.itemId, folder,
     "Found the expected folder in the right pane");
  
  library.ContentTree.view.selectNode(folderNode);

  synthesizeClickOnSelectedTreeCell(library.ContentTree.view,
                                    { clickCount: 2 });

  is(library.ContentTree.view.view.nodeForTreeIndex(0).itemId, bookmark,
     "Found the expected bookmark in the right pane");
});
