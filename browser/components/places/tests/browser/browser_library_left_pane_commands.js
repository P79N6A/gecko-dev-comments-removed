









































const TEST_URI = "http://www.mozilla.org/";

var gTests = [];
var gLibrary;



gTests.push({
  desc: "Bug 489351 - Date containers under History in Library cannot be deleted/cut",
  run: function() {
    var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
    
    PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                                 null, PlacesUtils.history.TRANSITION_TYPED,
                                 false, 0);
    ok(bhist.isVisited(PlacesUtils._uri(TEST_URI)), "Visit has been added");

    
    var PO = gLibrary.PlacesOrganizer;
    PO.selectLeftPaneQuery('History');
    isnot(PO._places.selectedNode, null, "We correctly selected History");

    
    ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
       "Cut command is disabled");
    ok(!PO._places.controller.isCommandEnabled("cmd_delete"),
       "Delete command is disabled");
    var historyNode = PO._places.selectedNode
                        .QueryInterface(Ci.nsINavHistoryContainerResultNode);
    historyNode.containerOpen = true;

    
    is(historyNode.childCount, 1, "History node has one child");
    var todayNode = historyNode.getChild(0);
    var todayNodeExpectedTitle = PlacesUtils.getString("finduri-AgeInDays-is-0");
    is(todayNode.title, todayNodeExpectedTitle,
       "History child is the expected container");

    
    PO._places.selectNode(todayNode);
    is(PO._places.selectedNode, todayNode,
       "We correctly selected Today container");
    
    ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
       "Cut command is disabled");
    ok(PO._places.controller.isCommandEnabled("cmd_delete"),
       "Delete command is enabled");

    
    PO._places.controller.doCommand("cmd_delete");
    ok(!bhist.isVisited(PlacesUtils._uri(TEST_URI)), "Visit has been removed");

    
    is(historyNode.childCount, 0, "History node has no more children");

    historyNode.containerOpen = false;
    nextTest();
  }
});



gTests.push({
  desc: "Bug 490156 - Can't delete smart bookmark containers",
  run: function() {
    
    var PO = gLibrary.PlacesOrganizer;
    PO.selectLeftPaneQuery('BookmarksToolbar');
    isnot(PO._places.selectedNode, null, "We have a valid selection");
    is(PlacesUtils.getConcreteItemId(PO._places.selectedNode),
       PlacesUtils.toolbarFolderId,
       "We have correctly selected bookmarks toolbar node.");

    
    ok(!PO._places.controller.isCommandEnabled("cmd_cut"),
       "Cut command is disabled");
    ok(!PO._places.controller.isCommandEnabled("cmd_delete"),
       "Delete command is disabled");

    var toolbarNode = PO._places.selectedNode
                        .QueryInterface(Ci.nsINavHistoryContainerResultNode);
    toolbarNode.containerOpen = true;

    
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                         PlacesUtils._uri("place:sort=4"),
                                         0, 
                                         "special_query");
    
    ok(toolbarNode.childCount > 0, "Toolbar node has children");
    var queryNode = toolbarNode.getChild(0);
    is(queryNode.title, "special_query", "Query node is correctly selected");

    
    PO._places.selectNode(queryNode);
    is(PO._places.selectedNode, queryNode, "We correctly selected query node");

    
    ok(PO._places.controller.isCommandEnabled("cmd_cut"),
       "Cut command is enabled");
    ok(PO._places.controller.isCommandEnabled("cmd_delete"),
       "Delete command is enabled");

    
    PO._places.controller.doCommand("cmd_delete");
    try {
      PlacesUtils.bookmarks.getFolderIdForItem(queryNode.itemId);  
      ok(false, "Unable to remove query node bookmark");
    } catch(ex) {
      ok(true, "Query node bookmark has been correctly removed");
    }

    toolbarNode.containerOpen = false;
    nextTest();
  }
});



function nextTest() {
  if (gTests.length) {
    var test = gTests.shift();
    info("Start of test: " + test.desc);
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
    nextTest();
  });
}
