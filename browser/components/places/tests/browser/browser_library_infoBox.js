








const TEST_URI = "http://www.mozilla.org/";

var gTests = [];
var gLibrary;



gTests.push({
  desc: "Bug 430148 - Remove or hide the more/less button in details pane...",
  run: function() {
    var PO = gLibrary.PlacesOrganizer;
    let ContentTree = gLibrary.ContentTree;
    var infoBoxExpanderWrapper = getAndCheckElmtById("infoBoxExpanderWrapper");

    function addVisitsCallback() {
      
      PO.selectLeftPaneQuery("AllBookmarks");
      isnot(PO._places.selectedNode, null,
            "Correctly selected all bookmarks node.");
      checkInfoBoxSelected(PO);
      ok(infoBoxExpanderWrapper.hidden,
         "Expander button is hidden for all bookmarks node.");
      checkAddInfoFieldsCollapsed(PO);

      
      PO.selectLeftPaneQuery("History");
      isnot(PO._places.selectedNode, null, "Correctly selected history node.");
      checkInfoBoxSelected(PO);
      ok(infoBoxExpanderWrapper.hidden,
         "Expander button is hidden for history node.");
      checkAddInfoFieldsCollapsed(PO);

      
      var historyNode = PO._places.selectedNode.
                        QueryInterface(Ci.nsINavHistoryContainerResultNode);
      historyNode.containerOpen = true;
      var childNode = historyNode.getChild(0);
      isnot(childNode, null, "History node first child is not null.");
      PO._places.selectNode(childNode);
      checkInfoBoxSelected(PO);
      ok(infoBoxExpanderWrapper.hidden,
         "Expander button is hidden for history child node.");
      checkAddInfoFieldsCollapsed(PO);

      
      var view = ContentTree.view.view;
      ok(view.rowCount > 0, "History item exists.");
      view.selection.select(0);
      ok(infoBoxExpanderWrapper.hidden,
         "Expander button is hidden for history item.");
      checkAddInfoFieldsCollapsed(PO);

      historyNode.containerOpen = false;

      
      PO.selectLeftPaneQuery("BookmarksMenu");
      isnot(PO._places.selectedNode, null,
            "Correctly selected bookmarks menu node.");
      checkInfoBoxSelected(PO);
      ok(infoBoxExpanderWrapper.hidden,
         "Expander button is hidden for bookmarks menu node.");
      checkAddInfoFieldsCollapsed(PO);

      
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                           NetUtil.newURI("place:folder=BOOKMARKS_MENU" +
                                                          "&folder=UNFILED_BOOKMARKS" +
                                                          "&folder=TOOLBAR" +
                                                          "&queryType=" + Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS +
                                                          "&sort=" + Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING +
                                                          "&maxResults=10" +
                                                          "&excludeQueries=1"),
                                           0, "Recent Bookmarks");
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                           NetUtil.newURI("http://mozilla.org/"),
                                           1, "Mozilla");
      var menuNode = PO._places.selectedNode.
                     QueryInterface(Ci.nsINavHistoryContainerResultNode);
      menuNode.containerOpen = true;
      childNode = menuNode.getChild(0);
      isnot(childNode, null, "Bookmarks menu child node exists.");
      is(childNode.title, "Recent Bookmarks",
         "Correctly selected recently bookmarked node.");
      PO._places.selectNode(childNode);
      checkInfoBoxSelected(PO);
      ok(!infoBoxExpanderWrapper.hidden,
         "Expander button is not hidden for recently bookmarked node.");
      checkAddInfoFieldsNotCollapsed(PO);

      
      var view = ContentTree.view.view;
      ok(view.rowCount > 0, "Bookmark item exists.");
      view.selection.select(0);
      checkInfoBoxSelected(PO);
      ok(!infoBoxExpanderWrapper.hidden,
         "Expander button is not hidden for bookmark item.");
      checkAddInfoFieldsNotCollapsed(PO);
      checkAddInfoFields(PO, "bookmark item");

      menuNode.containerOpen = false;

      PlacesTestUtils.clearHistory().then(nextTest);
    }
    
    addVisits(
      { uri: PlacesUtils._uri(TEST_URI), visitDate: Date.now() * 1000,
        transition: PlacesUtils.history.TRANSITION_TYPED },
      window,
      addVisitsCallback);
  }
});

function checkInfoBoxSelected(PO) {
  is(getAndCheckElmtById("detailsDeck").selectedIndex, 1,
     "Selected element in detailsDeck is infoBox.");
}

function checkAddInfoFieldsCollapsed(PO) {
  PO._additionalInfoFields.forEach(function (id) {
    ok(getAndCheckElmtById(id).collapsed,
       "Additional info field correctly collapsed: #" + id);
  });
}

function checkAddInfoFieldsNotCollapsed(PO) {
  ok(PO._additionalInfoFields.some(function (id) {
      return !getAndCheckElmtById(id).collapsed;
     }), "Some additional info field correctly not collapsed");
}

function checkAddInfoFields(PO, nodeName) {
  ok(true, "Checking additional info fields visibiity for node: " + nodeName);
  var expanderButton = getAndCheckElmtById("infoBoxExpander");

  
  PO._additionalInfoFields.forEach(function (id) {
    ok(getAndCheckElmtById(id).hidden,
       "Additional info field correctly hidden by default: #" + id);
  });

  
  expanderButton.click();
  PO._additionalInfoFields.forEach(function (id) {
    ok(!getAndCheckElmtById(id).hidden,
       "Additional info field correctly unhidden after toggle: #" + id);
  });
  expanderButton.click();
  PO._additionalInfoFields.forEach(function (id) {
    ok(getAndCheckElmtById(id).hidden,
       "Additional info field correctly hidden after toggle: #" + id);
  });
}

function getAndCheckElmtById(id) {
  var elmt = gLibrary.document.getElementById(id);
  isnot(elmt, null, "Correctly got element: #" + id);
  return elmt;
}



function nextTest() {
  if (gTests.length) {
    var test = gTests.shift();
    ok(true, "TEST: " + test.desc);
    dump("TEST: " + test.desc + "\n");
    test.run();
  }
  else {
    
    gLibrary.close();
    
    finish();
  }
}

function test() {
  waitForExplicitFinish();
  
  ok(PlacesUtils, "PlacesUtils is running in chrome context");
  ok(PlacesUIUtils, "PlacesUIUtils is running in chrome context");

  
  openLibrary(function (library) {
    gLibrary = library;
    gLibrary.PlacesOrganizer._places.focus();
    nextTest(gLibrary);
  });
}
