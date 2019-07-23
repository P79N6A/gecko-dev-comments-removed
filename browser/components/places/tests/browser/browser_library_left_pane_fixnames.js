









































var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);





var leftPaneQueries = [];

var windowObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic === "domwindowopened") {
      ww.unregisterNotification(this);
      var organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
      organizer.addEventListener("load", function onLoad(event) {
        organizer.removeEventListener("load", onLoad, false);
        executeSoon(function () {
          
          for (var i = 0; i < leftPaneQueries.length; i++) {
            var query = leftPaneQueries[i];
            is(PlacesUtils.bookmarks.getItemTitle(query.itemId),
               query.correctTitle, "Title is correct for query " + query.name);
            if ("concreteId" in query) {
              is(PlacesUtils.bookmarks.getItemTitle(query.concreteId),
               query.concreteTitle, "Concrete title is correct for query " + query.name);
            }
          }

          
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

  
  ok(PlacesUIUtils.leftPaneFolderId > 0, "left pane folder is initialized");

  
  var leftPaneItems = PlacesUtils.annotations
                                 .getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO);

  is(leftPaneItems.length, 1, "We correctly have only 1 left pane folder");
  
  var version = PlacesUtils.annotations
                           .getItemAnnotation(leftPaneItems[0],
                                              ORGANIZER_FOLDER_ANNO);
  is(version, ORGANIZER_LEFTPANE_VERSION, "Left pane version is actual");

  
  var items = PlacesUtils.annotations
                         .getItemsWithAnnotation(ORGANIZER_QUERY_ANNO);
  
  for (var i = 0; i < items.length; i++) {
    var itemId = items[i];
    var queryName = PlacesUtils.annotations
                               .getItemAnnotation(items[i],
                                                  ORGANIZER_QUERY_ANNO);
    var query = { name: queryName,
                  itemId: itemId,
                  correctTitle: PlacesUtils.bookmarks.getItemTitle(itemId) }
    switch (queryName) {
      case "BookmarksToolbar":
        query.concreteId = PlacesUtils.toolbarFolderId;
        query.concreteTitle = PlacesUtils.bookmarks.getItemTitle(query.concreteId);
        break;
      case "BookmarksMenu":
        query.concreteId = PlacesUtils.bookmarksMenuFolderId;
        query.concreteTitle = PlacesUtils.bookmarks.getItemTitle(query.concreteId);
        break;
      case "UnfiledBookmarks":
        query.concreteId = PlacesUtils.unfiledBookmarksFolderId;
        query.concreteTitle = PlacesUtils.bookmarks.getItemTitle(query.concreteId);
        break;
    }
    leftPaneQueries.push(query);
    
    PlacesUtils.bookmarks.setItemTitle(query.itemId, "badName");
    if ("concreteId" in query)
      PlacesUtils.bookmarks.setItemTitle(query.concreteId, "badName");
  }

  
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
