

















const TEST_URI = "http://www.mozilla.org/";

function test() {
  function onLibraryReady(organizer) {
    let contentTree = organizer.document.getElementById("placeContent");
    isnot(contentTree, null, "Sanity check: placeContent tree should exist");
    isnot(organizer.PlacesOrganizer, null, "Sanity check: PlacesOrganizer should exist");
    isnot(organizer.gEditItemOverlay, null, "Sanity check: gEditItemOverlay should exist");

    ok(organizer.gEditItemOverlay.initialized, "gEditItemOverlay is initialized");
    isnot(organizer.gEditItemOverlay.itemId, -1, "Editing a bookmark");

    
    organizer.PlacesOrganizer.selectLeftPaneQuery('History');
    
    let selection = contentTree.view.selection;
    selection.clearSelection();
    selection.rangedSelect(0, 0, true);
    
    is(organizer.gEditItemOverlay.itemId, -1, "Editing an history entry");
    
    organizer.close();
    
    PlacesTestUtils.clearHistory().then(finish);
  }

  waitForExplicitFinish();
  
  ok(PlacesUtils, "checking PlacesUtils, running in chrome context?");
  PlacesTestUtils.addVisits(
    {uri: PlacesUtils._uri(TEST_URI), visitDate: Date.now() * 1000,
      transition: PlacesUtils.history.TRANSITION_TYPED}
    ).then(() => {
      openLibrary(onLibraryReady);
    });
}
