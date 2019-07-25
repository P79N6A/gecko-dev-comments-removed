










































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
    openLibrary(function (organizer) {
          
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
            contextmenu.removeEventListener("popupshown", arguments.callee, true);
            let forgetThisSite = doc.getElementById("placesContext_deleteHost");
            let hideForgetThisSite = (selectionCount != 1);
            is(forgetThisSite.hidden, hideForgetThisSite,
              "The Forget this site menu item should " + (hideForgetThisSite ? "" : "not ") +
              "be hidden with " + selectionCount + " items selected");
            
            contextmenu.hidePopup();
            
            organizer.addEventListener("unload", function () {
              organizer.removeEventListener("unload", arguments.callee, false);
              
              funcNext();
            }, false);
            
            organizer.close();
          }, true);
          
          var x = {}, y = {}, width = {}, height = {};
          tree.treeBoxObject.getCoordsForCellItem(0, tree.columns[0], "text",
                                                  x, y, width, height);
          
          EventUtils.synthesizeMouse(tree.body, x.value + width.value / 2, y.value + height.value / 2, {type: "contextmenu"}, organizer);
    });
  }

  testForgetThisSiteVisibility(1, function() {
    testForgetThisSiteVisibility(2, function() {
      
      waitForClearHistory(finish);
    });
  });
}
