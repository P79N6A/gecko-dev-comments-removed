











































const TEST_URI = "http://www.mozilla.org/";

var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

var gFakeLeftPanes = [];

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
             "We correctly have only 1 left pane folder");

          
          gFakeLeftPanes.forEach(function(aItemId) {
            try {
              PlacesUtils.bookmarks.getItemTitle(aItemId);
              throw("This folder should have been removed");
            } catch (ex) {}
          });

          
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

  
  do {
    let leftPaneItems = PlacesUtils.annotations
                                 .getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
    
    let fakeLeftPaneRoot =
      PlacesUtils.bookmarks.createFolder(PlacesUtils.placesRootId, "",
                                         PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.annotations.setItemAnnotation(fakeLeftPaneRoot,
                                              ORGANIZER_FOLDER_ANNO,
                                              ORGANIZER_LEFTPANE_VERSION,
                                              0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
    gFakeLeftPanes.push(fakeLeftPaneRoot);
  } while (gFakeLeftPanes.length < 2);

  
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
