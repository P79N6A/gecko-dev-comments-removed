

















































const TEST_URI = "http://www.mozilla.org/";

let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

let windowObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic === "domwindowopened") {
      ww.unregisterNotification(this);
      let organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
      organizer.addEventListener("load", function onLoad(event) {
        organizer.removeEventListener("load", onLoad, false);
        executeSoon(function () {
          let contentTree = organizer.document.getElementById("placeContent");
          isnot(contentTree, null, "Sanity check: placeContent tree should exist");
          isnot(organizer.PlacesOrganizer, null, "Sanity check: PlacesOrganizer should exist");
          isnot(organizer.gEditItemOverlay, null, "Sanity check: gEditItemOverlay should exist");
          isnot(organizer.gEditItemOverlay.itemId, -1, "Editing a bookmark");
          
          organizer.PlacesOrganizer.selectLeftPaneQuery('History');
          
          let selection = contentTree.view.selection;
          selection.clearSelection();
          selection.rangedSelect(0, 0, true);
          
          is(organizer.gEditItemOverlay.itemId, -1, "Editing an history entry");
          
          organizer.close();
          
          PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
          finish();
        });
      }, false);
    }
  }
};

function test() {
  waitForExplicitFinish();
  
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                               null, PlacesUtils.history.TRANSITION_TYPED,
                               false, 0);

  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
