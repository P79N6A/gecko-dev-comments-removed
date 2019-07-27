add_task(function* () {
  
  

  
  let root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;

  function* insertAndTest(bmInfo) {
    bmInfo = yield PlacesUtils.bookmarks.insert(bmInfo);
    let node = root.getChild(root.childCount - 1);
    Assert.equal(node.bookmarkGuid, bmInfo.guid);
    Assert.equal(node.dateAdded, bmInfo.dateAdded * 1000);
    Assert.equal(node.lastModified, bmInfo.lastModified * 1000);
  }

  
  yield insertAndTest({ parentGuid: root.bookmarkGuid
                      , type: PlacesUtils.bookmarks.TYPE_BOOKMARK
                      , title: "Test Bookmark"
                      , url: "http://test.url.tld" });

  
  yield insertAndTest({ parentGuid: root.bookmarkGuid
                      , type: PlacesUtils.bookmarks.TYPE_BOOKMARK
                      , title: "Test Query"
                      , url: "place:folder=BOOKMARKS_MENU" });

  
  yield insertAndTest({ parentGuid: root.bookmarkGuid
                      , type: PlacesUtils.bookmarks.TYPE_FOLDER
                      , title: "Test Folder" });

  
  yield insertAndTest({ parentGuid: root.bookmarkGuid
                      , type: PlacesUtils.bookmarks.TYPE_SEPARATOR });

  root.containerOpen = false;
});

function run_test() {
  run_next_test();
}
