










let gLeftPaneFolderIdGetter;
let gAllBookmarksFolderIdGetter;

let gReferenceHierarchy;
let gLeftPaneFolderId;

add_task(function* () {
  
  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!!PlacesUIUtils);

  
  gLeftPaneFolderIdGetter = Object.getOwnPropertyDescriptor(PlacesUIUtils, "leftPaneFolderId");
  Assert.equal(typeof(gLeftPaneFolderIdGetter.get), "function");
  gAllBookmarksFolderIdGetter = Object.getOwnPropertyDescriptor(PlacesUIUtils, "allBookmarksFolderId");
  Assert.equal(typeof(gAllBookmarksFolderIdGetter.get), "function");

  do_register_cleanup(() => PlacesUtils.bookmarks.eraseEverything());
});

add_task(function* () {
  
  let folder = yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.unfiledGuid,
    title: "test",
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    type: PlacesUtils.bookmarks.TYPE_FOLDER
  });

  let folderId = yield PlacesUtils.promiseItemId(folder.guid);
  PlacesUtils.annotations.setItemAnnotation(folderId, ORGANIZER_QUERY_ANNO,
                                            "test", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  
  gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
  gReferenceHierarchy = folderIdToHierarchy(gLeftPaneFolderId);

  while (gTests.length) {
    
    yield Task.spawn(gTests.shift());

    
    Object.defineProperty(PlacesUIUtils, "leftPaneFolderId", gLeftPaneFolderIdGetter);
    gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
    Object.defineProperty(PlacesUIUtils, "allBookmarksFolderId", gAllBookmarksFolderIdGetter);

    
    let leftPaneHierarchy = folderIdToHierarchy(gLeftPaneFolderId)
    Assert.equal(gReferenceHierarchy, leftPaneHierarchy);

    folder = yield PlacesUtils.bookmarks.fetch({guid: folder.guid});
    Assert.equal(folder.title, "test");
  }
});


let gTests = [

  function* test1() {
    print("1. Do nothing, checks test calibration.");
  },

  function* test2() {
    print("2. Delete the left pane folder.");
    let guid = yield PlacesUtils.promiseItemGuid(gLeftPaneFolderId);
    yield PlacesUtils.bookmarks.remove(guid);
  },

  function* test3() {
    print("3. Delete a child of the left pane folder.");
    let guid = yield PlacesUtils.promiseItemGuid(gLeftPaneFolderId);
    let bm = yield PlacesUtils.bookmarks.fetch({parentGuid: guid, index: 0});
    yield PlacesUtils.bookmarks.remove(bm.guid);
  },

  function* test4() {
    print("4. Delete AllBookmarks.");
    let guid = yield PlacesUtils.promiseItemGuid(PlacesUIUtils.allBookmarksFolderId);
    yield PlacesUtils.bookmarks.remove(guid);
  },

  function* test5() {
    print("5. Create a duplicated left pane folder.");
    let folder = yield PlacesUtils.bookmarks.insert({
      parentGuid: PlacesUtils.bookmarks.unfiledGuid,
      title: "PlacesRoot",
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      type: PlacesUtils.bookmarks.TYPE_FOLDER
    });

    let folderId = yield PlacesUtils.promiseItemId(folder.guid);
    PlacesUtils.annotations.setItemAnnotation(folderId, ORGANIZER_FOLDER_ANNO,
                                              "PlacesRoot", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function* test6() {
    print("6. Create a duplicated left pane query.");
    let folder = yield PlacesUtils.bookmarks.insert({
      parentGuid: PlacesUtils.bookmarks.unfiledGuid,
      title: "AllBookmarks",
      index: PlacesUtils.bookmarks.DEFAULT_INDEX,
      type: PlacesUtils.bookmarks.TYPE_FOLDER
    });

    let folderId = yield PlacesUtils.promiseItemId(folder.guid);
    PlacesUtils.annotations.setItemAnnotation(folderId, ORGANIZER_QUERY_ANNO,
                                              "AllBookmarks", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function* test7() {
    print("7. Remove the left pane folder annotation.");
    PlacesUtils.annotations.removeItemAnnotation(gLeftPaneFolderId,
                                                 ORGANIZER_FOLDER_ANNO);
  },

  function* test8() {
    print("8. Remove a left pane query annotation.");
    PlacesUtils.annotations.removeItemAnnotation(PlacesUIUtils.allBookmarksFolderId,
                                                 ORGANIZER_QUERY_ANNO);
  },

  function* test9() {
    print("9. Remove a child of AllBookmarks.");
    let guid = yield PlacesUtils.promiseItemGuid(PlacesUIUtils.allBookmarksFolderId);
    let bm = yield PlacesUtils.bookmarks.fetch({parentGuid: guid, index: 0});
    yield PlacesUtils.bookmarks.remove(bm.guid);
  }

];




function folderIdToHierarchy(aFolderId) {
  let root = PlacesUtils.getFolderContents(aFolderId).root;
  let hier = JSON.stringify(hierarchyToObj(root));
  root.containerOpen = false;
  return hier;
}

function hierarchyToObj(aNode) {
  let o = {}
  o.title = aNode.title;
  o.annos = PlacesUtils.getAnnotationsForItem(aNode.itemId)
  if (PlacesUtils.nodeIsURI(aNode)) {
    o.uri = aNode.uri;
  }
  else if (PlacesUtils.nodeIsFolder(aNode)) {
    o.children = [];
    PlacesUtils.asContainer(aNode).containerOpen = true;
    for (let i = 0; i < aNode.childCount; ++i) {
      o.children.push(hierarchyToObj(aNode.getChild(i)));
    }
    aNode.containerOpen = false;
  }
  return o;
}
