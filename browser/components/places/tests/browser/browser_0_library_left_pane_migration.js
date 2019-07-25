











































const TEST_URI = "http://www.mozilla.org/";

function onLibraryReady(organizer) {
      
      ok(PlacesUIUtils.leftPaneFolderId > 0,
         "Left pane folder correctly created");
      var leftPaneItems =
        PlacesUtils.annotations
                   .getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
      is(leftPaneItems.length, 1,
         "We correctly have only 1 left pane folder");
      var leftPaneRoot = leftPaneItems[0];
      is(leftPaneRoot, PlacesUIUtils.leftPaneFolderId,
         "leftPaneFolderId getter has correct value");
      
      var version =
        PlacesUtils.annotations.getItemAnnotation(leftPaneRoot,
                                                  PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
      is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION,
         "Left pane version has been correctly upgraded");

      
      organizer.PlacesOrganizer.selectLeftPaneQuery('History');
      is(organizer.PlacesOrganizer._places.selectedNode.itemId,
         PlacesUIUtils.leftPaneQueries["History"],
         "Library left pane is populated and working");

      
      organizer.close();
      
      finish();
}

function test() {
  waitForExplicitFinish();
  
  ok(PlacesUtils, "PlacesUtils is running in chrome context");
  ok(PlacesUIUtils, "PlacesUIUtils is running in chrome context");
  ok(PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION > 0,
     "Left pane version in chrome context, current version is: " + PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION );

  
  var leftPaneItems = PlacesUtils.annotations
                                 .getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  if (leftPaneItems.length > 0) {
    
    
    is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
    
    var version = PlacesUtils.annotations.getItemAnnotation(leftPaneItems[0],
                                                            PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
    is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION, "Left pane version is actual");
    ok(true, "left pane has already been created, skipping test");
    finish();
    return;
  }

  
  var fakeLeftPaneRoot =
    PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId, "",
                                       PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(fakeLeftPaneRoot,
                                            PlacesUIUtils.ORGANIZER_FOLDER_ANNO,
                                            PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION - 1,
                                            0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  var leftPaneItems =
    PlacesUtils.annotations.getItemsWithAnnotation(PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
  is(leftPaneItems[0], fakeLeftPaneRoot, "left pane root itemId is correct");

  
  var version = PlacesUtils.annotations.getItemAnnotation(fakeLeftPaneRoot,
                                                          PlacesUIUtils.ORGANIZER_FOLDER_ANNO);
  is(version, PlacesUIUtils.ORGANIZER_LEFTPANE_VERSION - 1, "Left pane version correctly set");

  
  openLibrary(onLibraryReady);
}
