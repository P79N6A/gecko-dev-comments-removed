








































const TEST_URI = "http://www.mozilla.org/";

var gTests = [];
var gLibrary;



gTests.push({
  desc: "Bug 430148 - Remove or hide the more/less button in details pane...",
  run: function() {
    var PO = gLibrary.PlacesOrganizer;
    var infoBoxExpanderWrapper = getAndCheckElmtById("infoBoxExpanderWrapper");

    
    var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
    PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_URI), Date.now() * 1000,
                                 null, PlacesUtils.history.TRANSITION_TYPED,
                                 false, 0);
    ok(bhist.isVisited(PlacesUtils._uri(TEST_URI)), "Visit has been added.");

    
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

    
    var view = PO._content.treeBoxObject.view;
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

    
    var menuNode = PO._places.selectedNode.
                   QueryInterface(Ci.nsINavHistoryContainerResultNode);
    menuNode.containerOpen = true;
    childNode = menuNode.getChild(0);
    isnot(childNode, null, "Bookmarks menu child node exists.");
    var recentlyBookmarkedTitle = PlacesUIUtils.
                                  getString("recentlyBookmarkedTitle");
    isnot(recentlyBookmarkedTitle, null,
          "Correctly got the recently bookmarked title locale string.");
    is(childNode.title, recentlyBookmarkedTitle,
       "Correctly selected recently bookmarked node.");
    PO._places.selectNode(childNode);
    checkInfoBoxSelected(PO);
    ok(!infoBoxExpanderWrapper.hidden,
       "Expander button is not hidden for recently bookmarked node.");
    checkAddInfoFieldsNotCollapsed(PO);

    
    var view = PO._content.treeBoxObject.view;
    ok(view.rowCount > 0, "Bookmark item exists.");
    view.selection.select(0);
    checkInfoBoxSelected(PO);
    ok(!infoBoxExpanderWrapper.hidden,
       "Expander button is not hidden for bookmark item.");
    checkAddInfoFieldsNotCollapsed(PO);
    checkAddInfoFields(PO, "bookmark item");

    
    ok(view.rowCount > 1, "Second bookmark item exists.");
    view.selection.select(1);
    checkInfoBoxSelected(PO);
    ok(!infoBoxExpanderWrapper.hidden,
       "Expander button is not hidden for second bookmark item.");
    checkAddInfoFieldsNotCollapsed(PO);
    checkAddInfoFields(PO, "second bookmark item");

    menuNode.containerOpen = false;

    bhist.removeAllPages();
    nextTest();
  }
});

function checkInfoBoxSelected(PO) {
  PO._places.focus();
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

var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

var windowObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic === "domwindowopened") {
      ww.unregisterNotification(this);
      gLibrary = aSubject.QueryInterface(Ci.nsIDOMWindow);
      gLibrary.addEventListener("load", function onLoad(event) {
        gLibrary.removeEventListener("load", onLoad, false);
        executeSoon(function () {
          
          nextTest();
        });
      }, false);
    }
  }
};

function test() {
  dump("Starting test browser_library_infoBox.js\n");
  waitForExplicitFinish();
  
  ok(PlacesUtils, "PlacesUtils is running in chrome context");
  ok(PlacesUIUtils, "PlacesUIUtils is running in chrome context");

  
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
