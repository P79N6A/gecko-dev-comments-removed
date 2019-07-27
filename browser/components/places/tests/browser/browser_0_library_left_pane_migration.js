





"use strict";







const TEST_URI = "http://www.mozilla.org/";

add_task(function* () {
  
  ok(PlacesUtils, "PlacesUtils is running in chrome context");
  ok(PlacesUIUtils, "PlacesUIUtils is running in chrome context");
  ok(PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION > 0,
     "Left pane version in chrome context, current version is: " + PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION );

  
  let leftPaneItems = PlacesUtils.annotations
                                 .getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  if (leftPaneItems.length > 0) {
    
    
    is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
    
    let version = PlacesUtils.annotations.getItemAnnotation(leftPaneItems[0],
                                                            PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
    is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION, "Left pane version is actual");
    ok(true, "left pane has already been created, skipping test");
    return;
  }

  
  let folder = yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.rootGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    type: PlacesUtils.bookmarks.TYPE_FOLDER,
    title: ""
  });

  let folderId = yield PlacesUtils.promiseItemId(folder.guid);
  PlacesUtils.annotations.setItemAnnotation(folderId,
                                            PlacesUIUtils.ORGANIZER_FOLDER_ANNO,
                                            PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION - 1,
                                            0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  leftPaneItems =
    PlacesUtils.annotations.getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
  is(leftPaneItems[0], folderId, "left pane root itemId is correct");

  
  let version = PlacesUtils.annotations.getItemAnnotation(folderId,
                                                          PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION - 1, "Left pane version correctly set");

  
  let organizer = yield promiseLibrary();

  
  ok(PlacesUIUtils.leftPaneFolderId > 0, "Left pane folder correctly created");
  leftPaneItems =
    PlacesUtils.annotations.getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
  let leftPaneRoot = leftPaneItems[0];
  is(leftPaneRoot, PlacesUIUtils.leftPaneFolderId,
     "leftPaneFolderId getter has correct value");

  
  version = PlacesUtils.annotations.getItemAnnotation(leftPaneRoot,
                                                      PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION,
     "Left pane version has been correctly upgraded");

  
  organizer.PlacesOrganizer.selectLeftPaneQuery("History");
  is(organizer.PlacesOrganizer._places.selectedNode.itemId,
     PlacesUIUtils.leftPaneQueries["History"],
     "Library left pane is populated and working");

  yield promiseLibraryClosed(organizer);
});
