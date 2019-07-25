










































function waitForClearHistory(aCallback) {
  const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }
  };
  Services.obs.addObserver(observer, TOPIC_EXPIRATION_FINISHED, false);

  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  hs.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
}

function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  waitForExplicitFinish();

  
  const TEST_URI = "http://www.mozilla.org/privatebrowsing";
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  let history = PlacesUtils.history;
  let visitId = history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                                 null, PlacesUtils.history.TRANSITION_TYPED, false, 0);
  ok(visitId > 0, TEST_URI + " successfully marked visited");

  function testForgetThisSiteVisibility(expected, funcNext) {
    function observer(aSubject, aTopic, aData) {
      if (aTopic != "domwindowopened")
        return;

      Services.ww.unregisterNotification(observer);
      let organizer = aSubject.QueryInterface(Ci.nsIDOMWindow);
      SimpleTest.waitForFocus(function() {
        executeSoon(function() {
          
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
            contextmenu.removeEventListener("popupshown", arguments.callee, true);
            let forgetThisSite = doc.getElementById("placesContext_deleteHost");
            is(forgetThisSite.hidden, !expected,
              "The Forget This Site menu item should " + (expected ? "not " : "") + "be hidden");
            let forgetThisSiteCmd = doc.getElementById("placesCmd_deleteDataHost");
            if (forgetThisSiteCmd.disabled, !expected,
              "The Forget This Site command should " + (expected ? "not " : "") + "be disabled");
            
            contextmenu.hidePopup();
            
            function closeObserver(aSubject, aTopic, aData) {
              if (aTopic != "domwindowclosed")
                return;
              Services.ww.unregisterNotification(closeObserver);
              SimpleTest.waitForFocus(function() {
                
                funcNext();
              });
            }
            Services.ww.registerNotification(closeObserver);
            
            organizer.close();
          }, true);
          
          var x = {}, y = {}, width = {}, height = {};
          tree.treeBoxObject.getCoordsForCellItem(0, tree.columns[0], "text",
                                                  x, y, width, height);
          
          EventUtils.synthesizeMouse(tree.body, x.value + width.value / 2, y.value + height.value / 2, {type: "contextmenu"}, organizer);
        });
      }, organizer);
    }

    Services.ww.registerNotification(observer);
    Services.ww.openWindow(null,
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
        
        waitForClearHistory(finish);
      });
    });
  });
}
