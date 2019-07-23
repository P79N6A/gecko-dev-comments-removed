









































const TEST_URI = "http://www.mozilla.org/";

var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

var windowObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic === "domwindowopened") {
      ww.unregisterNotification(this);
      var organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
      organizer.addEventListener("load", function onLoad(event) {
        organizer.removeEventListener("load", onLoad, false);
        executeSoon(function () {
          
          ok(PlacesUIUtils.leftPaneFolderId > 0,
             "Left pane folder correctly created");
          var leftPaneItems =
            PlacesUtils.annotations
                       .getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
          is(leftPaneItems.length, 1,
             "We correctly have only 1 left panel folder");
          var leftPaneRoot = leftPaneItems[0];

          
          var version =
            PlacesUtils.annotations.getItemAnnotation(leftPaneRoot,
                                                      ORGANIZER_FOLDER_ANNO);
          is(version, ORGANIZER_LEFTPANE_VERSION,
             "Left pane version has been correctly upgraded");

          
          organizer.PlacesOrganizer.selectLeftPaneQuery('History');
          ok(organizer.PlacesOrganizer._places.selectedNode.itemId,
             "Library left pane is populated and working");

          
          organizer.close();
          
          finish();
        });
      }, false);
    }
  }
};

function test() {
  waitForExplicitFinish();
  
  ok(PlacesUtils, "PlacesUtils is running in chrome context");
  ok(PlacesUIUtils, "PlacesUIUtils is running in chrome context");
  ok(ORGANIZER_LEFTPANE_VERSION > 0,
     "Left pane version in chrome context, current version is: " + ORGANIZER_LEFTPANE_VERSION );

  
  var leftPaneItems = PlacesUtils.annotations
                                 .getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
  if (leftPaneItems.length == 1)
    PlacesUtils.bookmarks.removeItem(leftPaneItems[0]);

  
  var fakeLeftPaneRoot =
    PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId, "",
                                       PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(fakeLeftPaneRoot,
                                            ORGANIZER_FOLDER_ANNO,
                                            ORGANIZER_LEFTPANE_VERSION-1,
                                            0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  var leftPaneItems =
    PlacesUtils.annotations.getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
  is(leftPaneItems.length, 1, "We correctly have only 1 left panel folder");
  is(leftPaneItems[0], fakeLeftPaneRoot, "left pane root itemId is correct");

  
  var version = PlacesUtils.annotations.getItemAnnotation(fakeLeftPaneRoot,
                                                          ORGANIZER_FOLDER_ANNO);
  is(version, ORGANIZER_LEFTPANE_VERSION-1, "Left pane version correctly set");

  
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
