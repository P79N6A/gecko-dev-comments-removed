




































function test() {
  
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  ok(PlacesUIUtils, "checking PlacesUIUtils, running in chrome context?");

  









  var toolbarId = PlacesUtils.toolbarFolderId;
  var toolbarNode = PlacesUtils.getFolderContents(toolbarId).root;

  var oldCount = toolbarNode.childCount;
  var testRootId = PlacesUtils.bookmarks.createFolder(toolbarId, "test root", -1);
  is(toolbarNode.childCount, oldCount+1, "confirm test root node is a container, and is empty");
  var testRootNode = toolbarNode.getChild(toolbarNode.childCount-1);
  asContainer(testRootNode);
  testRootNode.containerOpen = true;
  is(testRootNode.childCount, 0, "confirm test root node is a container, and is empty");

  
  var folderAId = PlacesUtils.bookmarks.createFolder(testRootId, "A", -1);
  PlacesUtils.bookmarks.insertBookmark(folderAId, PlacesUtils._uri("http://foo"),
                                       -1, "test bookmark");
  PlacesUtils.bookmarks.insertSeparator(folderAId, -1);
  PlacesUtils.livemarks.createLivemarkFolderOnly(folderAId, "test livemark",
                                                 PlacesUtils._uri("http://test"),
                                                 PlacesUtils._uri("http://test"), -1);

  var folderANode = testRootNode.getChild(0);
  var folderAGUIDs = getGUIDs(folderANode);

  
  ok(checkGUIDs(folderANode, folderAGUIDs, true), "confirm guid test works");

  
  var serializedNode = PlacesUtils.wrapNode(folderANode, PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);
  var rawNode = PlacesUtils.unwrapNodes(serializedNode, PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER).shift();
  ok(rawNode.type, "confirm json node was made");

  
  
  var transaction = PlacesUIUtils.makeTransaction(rawNode,
                                                  PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                                                  testRootId, -1, true);
  ok(transaction, "create transaction");

  
  PlacesUIUtils.ptm.doTransaction(transaction);
  is(testRootNode.childCount, 2, "create test folder via copy");

  
  var folderBNode = testRootNode.getChild(1);
  ok(checkGUIDs(folderBNode, folderAGUIDs, false), "confirm folder A GUIDs don't match folder B GUIDs");
  var folderBGUIDs = getGUIDs(folderBNode);
  ok(checkGUIDs(folderBNode, folderBGUIDs, true), "confirm test of new GUIDs");

  
  PlacesUIUtils.ptm.undoTransaction();
  is(testRootNode.childCount, 1, "confirm undo removed the copied folder");

  
  
  PlacesUIUtils.ptm.redoTransaction();
  is(testRootNode.childCount, 2, "confirm redo re-copied the folder");
  folderBNode = testRootNode.getChild(1);
  ok(checkGUIDs(folderBNode, folderAGUIDs, false), "folder B GUIDs after undo/redo don't match folder A GUIDs"); 
  ok(checkGUIDs(folderBNode, folderBGUIDs, true), "folder B GUIDs after under/redo should match pre-undo/redo folder B GUIDs");

  
  testRootNode.containerOpen = false;
  toolbarNode.containerOpen = false;

  
  PlacesUIUtils.ptm.undoTransaction();
  PlacesUtils.bookmarks.removeItem(testRootId);
}

function getGUIDs(aNode) {
  asContainer(aNode);
  aNode.containerOpen = true;
  var GUIDs = {
    folder: PlacesUtils.bookmarks.getItemGUID(aNode.itemId),
    bookmark: PlacesUtils.bookmarks.getItemGUID(aNode.getChild(0).itemId),
    separator: PlacesUtils.bookmarks.getItemGUID(aNode.getChild(1).itemId),
    livemark: PlacesUtils.bookmarks.getItemGUID(aNode.getChild(2).itemId)
  };
  aNode.containerOpen = false;
  return GUIDs;
}

function checkGUIDs(aFolderNode, aGUIDs, aShouldMatch) {

  function check(aNode, aGUID, aEquals) {
    var nodeGUID = PlacesUtils.bookmarks.getItemGUID(aNode.itemId);
    return aEquals ? (nodeGUID == aGUID) : (nodeGUID != aGUID);
  }

  asContainer(aFolderNode);
  aFolderNode.containerOpen = true;

  var allMatch = check(aFolderNode, aGUIDs.folder, aShouldMatch) &&
                 check(aFolderNode.getChild(0), aGUIDs.bookmark, aShouldMatch) &&
                 check(aFolderNode.getChild(1), aGUIDs.separator, aShouldMatch) &&
                 check(aFolderNode.getChild(2), aGUIDs.livemark, aShouldMatch);

  aFolderNode.containerOpen = false;
  return allMatch;
}
