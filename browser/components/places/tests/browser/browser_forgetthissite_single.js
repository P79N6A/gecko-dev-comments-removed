



"use strict";

const TEST_URIs = [
  "http://www.mozilla.org/test1",
  "http://www.mozilla.org/test2"
];



add_task(function* () {
  
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");

  let places = [];
  let transition = PlacesUtils.history.TRANSITION_TYPED;
  TEST_URIs.forEach(uri => places.push({uri: PlacesUtils._uri(uri), transition}));

  yield PlacesTestUtils.addVisits(places);
  yield testForgetThisSiteVisibility(1);
  yield testForgetThisSiteVisibility(2);

  
  yield PlacesTestUtils.clearHistory();
});

let testForgetThisSiteVisibility = Task.async(function* (selectionCount) {
  let organizer = yield promiseLibrary();

  
  organizer.PlacesOrganizer.selectLeftPaneQuery("History");
  let PO = organizer.PlacesOrganizer;
  let histContainer = PO._places.selectedNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  histContainer.containerOpen = true;
  PO._places.selectNode(histContainer.getChild(0));

  
  let doc = organizer.document;
  let tree = doc.getElementById("placeContent");
  let selection = tree.view.selection;
  selection.clearSelection();
  selection.rangedSelect(0, selectionCount - 1, true);
  is(selection.count, selectionCount, "The selected range is as big as expected");

  
  let contextmenu = doc.getElementById("placesContext");
  let popupShown = promisePopupShown(contextmenu);

  
  let rect = tree.treeBoxObject.getCoordsForCellItem(0, tree.columns[0], "text");
  
  EventUtils.synthesizeMouse(tree.body, rect.x + rect.width / 2, rect.y + rect.height / 2, {type: "contextmenu", button: 2}, organizer);
  yield popupShown;

  let forgetThisSite = doc.getElementById("placesContext_deleteHost");
  let hideForgetThisSite = (selectionCount != 1);
  is(forgetThisSite.hidden, hideForgetThisSite,
    `The Forget this site menu item should ${hideForgetThisSite ? "" : "not "}` +
    ` be hidden with ${selectionCount} items selected`);

  
  contextmenu.hidePopup();

  
  yield promiseLibraryClosed(organizer);
});

function promisePopupShown(popup) {
  return new Promise(resolve => {
    popup.addEventListener("popupshown", function onShown() {
      popup.removeEventListener("popupshown", onShown, true);
      resolve();
    }, true);
  });
}
