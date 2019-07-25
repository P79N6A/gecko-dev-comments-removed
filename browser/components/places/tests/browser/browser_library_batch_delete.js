






const TEST_URL = "http://www.batch.delete.me/";

var gTests = [];
var gLibrary;



gTests.push({
  desc: "Create and batch remove bookmarks",
  run: function() {
    let testURI = makeURI(TEST_URL);
    PlacesUtils.history.runInBatchMode({
      runBatched: function (aUserData) {
        
        let folder = PlacesUtils.bookmarks.createFolder(
          PlacesUtils.unfiledBookmarksFolderId, "deleteme",
          PlacesUtils.bookmarks.DEFAULT_INDEX
        );
        PlacesUtils.bookmarks.createFolder(
          PlacesUtils.unfiledBookmarksFolderId, "keepme",
          PlacesUtils.bookmarks.DEFAULT_INDEX
        );
        for (let i = 0; i < 10; i++) {
          PlacesUtils.bookmarks.insertBookmark(folder,
                                               testURI,
                                               PlacesUtils.bookmarks.DEFAULT_INDEX,
                                               "bm" + i);
        }
      }
    }, null);

    
    let PO = gLibrary.PlacesOrganizer;
    PO.selectLeftPaneQuery("UnfiledBookmarks");
    isnot(PO._places.selectedNode, null, "Selected unsorted bookmarks");

    let unsortedNode = PlacesUtils.asContainer(PO._places.selectedNode);
    unsortedNode.containerOpen = true;
    is(unsortedNode.childCount, 2, "Unsorted node has 2 children");
    let folderNode = unsortedNode.getChild(0);
    is(folderNode.title, "deleteme", "Folder found in unsorted bookmarks");
    
    PO._places.selectNode(folderNode);
    is(PO._places.selectedNode.title, "deleteme", "Folder node selected");
    ok(PO._places.controller.isCommandEnabled("cmd_delete"),
       "Delete command is enabled");
    
    PO._places.controller.doCommand("cmd_delete");
    ok(!PlacesUtils.bookmarks.isBookmarked(testURI),
      "Bookmark has been correctly removed");
    
    is(unsortedNode.childCount, 1, "Unsorted node has 1 child");
    is(PO._places.selectedNode.title, "keepme", "Folder node selected");
    unsortedNode.containerOpen = false;
    nextTest();
  }
});



gTests.push({
  desc: "Ensure correct selection and functionality in Library",
  run: function() {
    let PO = gLibrary.PlacesOrganizer;
    
    PO.selectLeftPaneQuery("History");
    PO.selectLeftPaneQuery("UnfiledBookmarks");
    
    PO._content.selectNode(PO._content.result.root.getChild(0));
    is(PO._content.selectedNode.title, "keepme",
       "Found folder in content pane");
    
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         makeURI(TEST_URL),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "bm" + i);
    is(PO._content.result.root.childCount, 2,
       "Right pane was correctly updated");
    nextTest();
  }
});



function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function () {
    PlacesUtils.bookmarks
               .removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  });

  openLibrary(function (library) {
    gLibrary = library;
    executeSoon(nextTest);
  });
}

function nextTest() {
  if (gTests.length) {
    var test = gTests.shift();
    info("Start of test: " + test.desc);
    test.run();
  }
  else {
    
    gLibrary.close();
    executeSoon(finish);
  }
}
