







































function test() {
  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  waitForExplicitFinish();

  
  let TEST_URIs = ["http://www.mozilla.org/test1", "http://www.mozilla.org/test2"];
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  let history = PlacesUtils.history;
  TEST_URIs.forEach(function(TEST_URI) {
    let visitId = history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                                   null, PlacesUtils.history.TRANSITION_TYPED, false, 0);
    ok(visitId > 0, TEST_URI + " successfully marked visited");
  });

  function testForgetThisSiteVisibility(selectionCount, funcNext) {
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          let organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
          organizer.addEventListener("load", function onLoad(event) {
            organizer.removeEventListener("load", onLoad, false);
            executeSoon(function () {
              
              organizer.PlacesOrganizer.selectLeftPaneQuery('History');
              let PO = organizer.PlacesOrganizer;
              let histContainer = PO._places.selectedNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
              histContainer.containerOpen = true;
              PO._places.selectNode(histContainer.getChild(0));
              
              let doc = organizer.document;
              let tree = PO._content;
              let selection = tree.view.selection;
              selection.clearSelection();
              selection.rangedSelect(0, selectionCount - 1, true);
              is(selection.count, selectionCount,
                "The selected range is as big as expected");
              
              let contextmenu = doc.getElementById("placesContext");
              contextmenu.addEventListener("popupshown", function() {
                contextmenu.removeEventListener("popupshown", arguments.callee, false);
                let forgetThisSite = doc.getElementById("placesContext_deleteHost");
                let hideForgetThisSite = (selectionCount != 1);
                is(forgetThisSite.hidden, hideForgetThisSite,
                  "The Forget this site menu item should " + (hideForgetThisSite ? "" : "not ") +
                  "be hidden with " + selectionCount + " items selected");
                
                contextmenu.hidePopup();
                
                organizer.close();
                
                funcNext();
              }, false);
              let event = document.createEvent("MouseEvents");
              event.initMouseEvent("contextmenu", true, true, organizer, 0,
                                   0, 0, 0, 0, false, false, false, false,
                                   0, null);
              tree.dispatchEvent(event);
            });
          }, false);
        }
      }
    };

    ww.registerNotification(observer);
    ww.openWindow(null,
                  "chrome://browser/content/places/places.xul",
                  "",
                  "chrome,toolbar=yes,dialog=no,resizable",
                  null);
  }

  testForgetThisSiteVisibility(1, function() {
    testForgetThisSiteVisibility(2, function() {
      
      history.QueryInterface(Ci.nsIBrowserHistory)
             .removeAllPages();
      finish();
    });
  });
}
