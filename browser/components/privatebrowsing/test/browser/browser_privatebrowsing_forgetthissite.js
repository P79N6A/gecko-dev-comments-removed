







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  waitForExplicitFinish();

  
  const TEST_URI = "http://www.mozilla.org/privatebrowsing";
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  let history = PlacesUtils.history;
  let visitId = history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                                 null, PlacesUtils.history.TRANSITION_TYPED, false, 0);
  ok(visitId > 0, TEST_URI + " successfully marked visited");

  function testForgetThisSiteVisibility(expected, funcNext) {
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          let organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
          organizer.addEventListener("load", function onLoad(event) {
            organizer.removeEventListener("load", onLoad, false);
            executeSoon(function () {
              
              let PO = organizer.PlacesOrganizer;
              PO.selectLeftPaneQuery('History');
              let histContainer = PO._places.selectedNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
              histContainer.containerOpen = true;
              PO._places.selectNode(histContainer.getChild(0));
              
              let doc = organizer.document;
              let tree = PO._content;
              let selection = tree.view.selection;
              selection.clearSelection();
              selection.rangedSelect(0, 0, true);
              is(tree.selectedNode.uri, TEST_URI, "The correct history item has been selected");
              
              let contextmenu = doc.getElementById("placesContext");
              contextmenu.addEventListener("popupshown", function() {
                contextmenu.removeEventListener("popupshown", arguments.callee, false);
                let forgetThisSite = doc.getElementById("placesContext_deleteHost");
                is(forgetThisSite.hidden, !expected,
                  "The Forget This Site menu item should " + (expected ? "not " : "") + "be hidden");
                let forgetThisSiteCmd = doc.getElementById("placesCmd_deleteDataHost");
                if (forgetThisSiteCmd.disabled, !expected,
                  "The Forget This Site command should " + (expected ? "not " : "") + "be disabled");
                
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

  testForgetThisSiteVisibility(true, function() {
    
    pb.privateBrowsingEnabled = true;
    testForgetThisSiteVisibility(false, function() {
      
      pb.privateBrowsingEnabled = false;
      testForgetThisSiteVisibility(true, function() {
        
        history.QueryInterface(Ci.nsIBrowserHistory)
               .removeAllPages();
        finish();
      });
    });
  });
}
