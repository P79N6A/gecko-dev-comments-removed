





function test() {
  
  waitForExplicitFinish();

  
  let TEST_URIs = ["http://www.mozilla.org/test1", "http://www.mozilla.org/test2"];
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  let places = [];
  TEST_URIs.forEach(function(TEST_URI) {
    places.push({uri: PlacesUtils._uri(TEST_URI),
                 transition: PlacesUtils.history.TRANSITION_TYPED});
  });
  PlacesTestUtils.addVisits(places).then(() => {
    testForgetThisSiteVisibility(1, function() {
      testForgetThisSiteVisibility(2, function() {
        
        PlacesTestUtils.clearHistory().then(finish);
      });
    });
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
          
          var rect = tree.treeBoxObject.getCoordsForCellItem(0, tree.columns[0], "text");
          
          EventUtils.synthesizeMouse(tree.body, rect.x + rect.width / 2, rect.y + rect.height / 2, {type: "contextmenu"}, organizer);
    });
  }
}
