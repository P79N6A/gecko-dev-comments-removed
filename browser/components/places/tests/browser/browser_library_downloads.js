












































let now = Date.now();

function test() {
  waitForExplicitFinish();

  function onLibraryReady(win) {
    
    fastAddVisit("http://mozilla.com",
                  PlacesUtils.history.TRANSITION_TYPED);
    fastAddVisit("http://google.com",
                  PlacesUtils.history.TRANSITION_DOWNLOAD);
    fastAddVisit("http://en.wikipedia.org",
                  PlacesUtils.history.TRANSITION_TYPED);
    fastAddVisit("http://ubuntu.org",
                  PlacesUtils.history.TRANSITION_DOWNLOAD);

    
    isnot(win.PlacesOrganizer._places.selectedNode, null,
          "Downloads is present and selected");

    
    let tree = win.document.getElementById("placeContent");
    isnot(tree, null, "placeContent tree exists");

    
    var contentRoot = tree.result.root;
    var len = contentRoot.childCount;
    var testUris = ["http://ubuntu.org/", "http://google.com/"];
    for (var i = 0; i < len; i++) {
      is(contentRoot.getChild(i).uri, testUris[i],
          "Comparing downloads shown at index " + i);
    }

    win.close();
    waitForClearHistory(finish);
  }

  openLibrary(onLibraryReady, "Downloads");
}

function fastAddVisit(uri, transition) {
  PlacesUtils.history.addVisit(PlacesUtils._uri(uri), now++ * 1000,
                               null, transition, false, 0);
}
