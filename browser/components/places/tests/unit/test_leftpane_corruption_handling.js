










let gLeftPaneFolderIdGetter;
let gAllBookmarksFolderIdGetter;

let gReferenceHierarchy;
let gLeftPaneFolderId;

let gFolderId;


let gTests = [

  function test1() {
    print("1. Do nothing, checks test calibration.");
  },

  function test2() {
    print("2. Delete the left pane folder.");
    PlacesUtils.bookmarks.removeItem(gLeftPaneFolderId);
  },

  function test3() {
    print("3. Delete a child of the left pane folder.");
    let id = PlacesUtils.bookmarks.getIdForItemAt(gLeftPaneFolderId, 0);
    PlacesUtils.bookmarks.removeItem(id);
  },

  function test4() {
    print("4. Delete AllBookmarks.");
    PlacesUtils.bookmarks.removeItem(PlacesUIUtils.allBookmarksFolderId);
  },

  function test5() {
    print("5. Create a duplicated left pane folder.");
    let id = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                "PlacesRoot",
                                                PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.annotations.setItemAnnotation(id, ORGANIZER_FOLDER_ANNO,
                                              "PlacesRoot", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function test6() {
    print("6. Create a duplicated left pane query.");
    let id = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                "AllBookmarks",
                                                PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.annotations.setItemAnnotation(id, ORGANIZER_QUERY_ANNO,
                                              "AllBookmarks", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function test7() {
    print("7. Remove the left pane folder annotation.");
    PlacesUtils.annotations.removeItemAnnotation(gLeftPaneFolderId,
                                                 ORGANIZER_FOLDER_ANNO);
  },

  function test8() {
    print("8. Remove a left pane query annotation.");
    PlacesUtils.annotations.removeItemAnnotation(PlacesUIUtils.allBookmarksFolderId,
                                                 ORGANIZER_QUERY_ANNO);
  },

  function test9() {
    print("9. Remove a child of AllBookmarks.");
    let id = PlacesUtils.bookmarks.getIdForItemAt(PlacesUIUtils.allBookmarksFolderId, 0);
    PlacesUtils.bookmarks.removeItem(id);
  },

];

function run_test() {
  
  remove_all_bookmarks();

  
  do_check_true(!!PlacesUIUtils);

  
  gLeftPaneFolderIdGetter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId");
  do_check_eq(typeof(gLeftPaneFolderIdGetter), "function");
  gAllBookmarksFolderIdGetter = PlacesUIUtils.__lookupGetter__("allBookmarksFolderId");
  do_check_eq(typeof(gAllBookmarksFolderIdGetter), "function");

  
  gFolderId = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                 "test",
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(gFolderId, ORGANIZER_QUERY_ANNO,
                                            "test", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  
  gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
  gReferenceHierarchy = folderIdToHierarchy(gLeftPaneFolderId);
  do_test_pending();
  run_next_test();
}

function run_next_test() {
  if (gTests.length) {
    
    let test = gTests.shift();
    test();
    
    PlacesUIUtils.__defineGetter__("leftPaneFolderId", gLeftPaneFolderIdGetter);
    gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
    PlacesUIUtils.__defineGetter__("allBookmarksFolderId", gAllBookmarksFolderIdGetter);
    
    Task.spawn(function() {
      let leftPaneHierarchy = folderIdToHierarchy(gLeftPaneFolderId)
      if (gReferenceHierarchy != leftPaneHierarchy) {
        do_throw("hierarchies differ!\n" + gReferenceHierarchy +
                                    "\n" + leftPaneHierarchy);
      }
      do_check_eq(PlacesUtils.bookmarks.getItemTitle(gFolderId), "test");
      
      run_next_test();
    });
  }
  else {
    
    remove_all_bookmarks();
    do_test_finished();
  }
}




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
