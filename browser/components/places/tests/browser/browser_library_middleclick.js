




































 



const ENABLE_HISTORY_PREF = "places.history.enabled";

var gLibrary = null;
var gTests = [];
var gCurrentTest = null;


var gTabsListener = {
  _loadedURIs: [],
  _openTabsCount: 0,

  handleEvent: function(aEvent) {
    if (aEvent.type != "TabOpen")
      return;

    if (++this._openTabsCount == gCurrentTest.URIs.length) {
      is(gBrowser.tabs.length, gCurrentTest.URIs.length + 1,
         "We have opened " + gCurrentTest.URIs.length + " new tab(s)");
    }

    var tab = aEvent.target;
    is(tab.ownerDocument.defaultView, window,
       "Tab has been opened in current browser window");
  },

  onLocationChange: function(aBrowser, aWebProgress, aRequest, aLocationURI) {
    var spec = aLocationURI.spec;
    ok(true, spec);
    
    
    
    if (spec == "about:blank" || this._loadedURIs.indexOf(spec) != -1)
      return;

    ok(gCurrentTest.URIs.indexOf(spec) != -1,
       "Opened URI found in list: " + spec);

    if (gCurrentTest.URIs.indexOf(spec) != -1 )
      this._loadedURIs.push(spec);

    if (this._loadedURIs.length == gCurrentTest.URIs.length) {
      

      
      this._loadedURIs.length = 0;
      
      while (gBrowser.tabs.length > 1)
        gBrowser.removeCurrentTab();
      this._openTabsCount = 0;

      
      waitForFocus(gCurrentTest.finish, gBrowser.ownerDocument.defaultView);
    }
  }
}




gTests.push({
  desc: "Open bookmark in a new tab.",
  URIs: ["about:buildconfig"],
  _itemId: -1,

  setup: function() {
    var bs = PlacesUtils.bookmarks;
    
    this._itemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                     PlacesUtils._uri(this.URIs[0]),
                                     bs.DEFAULT_INDEX,
                                     "Title");
    
    gLibrary.PlacesOrganizer.selectLeftPaneQuery("UnfiledBookmarks");
    isnot(gLibrary.PlacesOrganizer._places.selectedNode, null,
          "We correctly have selection in the Library left pane");
    
    var bookmarkNode = gLibrary.PlacesOrganizer._content.view.nodeForTreeIndex(0);
    is(bookmarkNode.uri, this.URIs[0], "Found bookmark in the right pane");
  },

  finish: function() {
    setTimeout(runNextTest, 0);
  },

  cleanup: function() {
    PlacesUtils.bookmarks.removeItem(this._itemId);
  }
});




gTests.push({
  desc: "Open a folder in tabs.",
  URIs: ["about:buildconfig", "about:"],
  _folderId: -1,

  setup: function() {
    var bs = PlacesUtils.bookmarks;
    
    var folderId = bs.createFolder(bs.unfiledBookmarksFolder,
                                   "Folder",
                                   bs.DEFAULT_INDEX);
    this._folderId = folderId;

    
    this.URIs.forEach(function(aURI) {
      bs.insertBookmark(folderId,
                        PlacesUtils._uri(aURI),
                        bs.DEFAULT_INDEX,
                        "Title");
    });

    
    gLibrary.PlacesOrganizer.selectLeftPaneQuery("UnfiledBookmarks");
    isnot(gLibrary.PlacesOrganizer._places.selectedNode, null,
          "We correctly have selection in the Library left pane");
    
    var folderNode = gLibrary.PlacesOrganizer._content.view.nodeForTreeIndex(0);
    is(folderNode.title, "Folder", "Found folder in the right pane");
  },

  finish: function() {
    setTimeout(runNextTest, 0);
  },

  cleanup: function() {
    PlacesUtils.bookmarks.removeItem(this._folderId);
  }
});




gTests.push({
  desc: "Open a query in tabs.",
  URIs: ["about:buildconfig", "about:"],
  _folderId: -1,
  _queryId: -1,

  setup: function() {
    var bs = PlacesUtils.bookmarks;
    
    var folderId = bs.createFolder(bs.unfiledBookmarksFolder,
                                   "Folder",
                                   bs.DEFAULT_INDEX);
    this._folderId = folderId;

    
    this.URIs.forEach(function(aURI) {
      bs.insertBookmark(folderId,
                        PlacesUtils._uri(aURI),
                        bs.DEFAULT_INDEX,
                        "Title");
    });

    
    var hs = PlacesUtils.history;
    var options = hs.getNewQueryOptions();
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    var query = hs.getNewQuery();
    
    
    query.searchTerms = "about:";
    var queryString = hs.queriesToQueryString([query], 1, options);
    this._queryId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                     PlacesUtils._uri(queryString),
                                     0, 
                                     "Query");

    
    gLibrary.PlacesOrganizer.selectLeftPaneQuery("UnfiledBookmarks");
    isnot(gLibrary.PlacesOrganizer._places.selectedNode, null,
          "We correctly have selection in the Library left pane");
    
    var folderNode = gLibrary.PlacesOrganizer._content.view.nodeForTreeIndex(0);
    is(folderNode.title, "Query", "Found query in the right pane");
  },

  finish: function() {
    setTimeout(runNextTest, 0);
  },

  cleanup: function() {
    PlacesUtils.bookmarks.removeItem(this._folderId);
    PlacesUtils.bookmarks.removeItem(this._queryId);
  }
});



function test() {
  waitForExplicitFinish();
  
  requestLongerTimeout(2);

  
  ok(PlacesUtils, "PlacesUtils in context");
  ok(PlacesUIUtils, "PlacesUIUtils in context");

  
  gBrowser.tabContainer.addEventListener("TabOpen", gTabsListener, false);
  gBrowser.addTabsProgressListener(gTabsListener);

  
  gPrefService.setBoolPref(ENABLE_HISTORY_PREF, false);

  
  openLibrary(function (library) {
    gLibrary = library;
    
    runNextTest();
  });
}

function runNextTest() {
  
  if (gCurrentTest)
    gCurrentTest.cleanup();

  if (gTests.length > 0) {
    
    gCurrentTest = gTests.shift();
    info("Start of test: " + gCurrentTest.desc);
    
    
    gCurrentTest.setup();

    
    gLibrary.focus();
    waitForFocus(function() {
      mouseEventOnCell(gLibrary.PlacesOrganizer._content, 0, 0, { button: 1 });
    }, gLibrary);
  }
  else {
    

    
    gLibrary.close();

    
    gBrowser.tabContainer.removeEventListener("TabOpen", gTabsListener, false);
    gBrowser.removeTabsProgressListener(gTabsListener);

    
    try {
      gPrefService.clearUserPref(ENABLE_HISTORY_PREF);
    } catch(ex) {}

    finish();
  }
}

function mouseEventOnCell(aTree, aRowIndex, aColumnIndex, aEventDetails) {
  var selection = aTree.view.selection;
  selection.select(aRowIndex);
  aTree.treeBoxObject.ensureRowIsVisible(aRowIndex);
  var column = aTree.columns[aColumnIndex];

  
  var x = {}, y = {}, width = {}, height = {};
  aTree.treeBoxObject.getCoordsForCellItem(aRowIndex, column, "text",
                                           x, y, width, height);

  EventUtils.synthesizeMouse(aTree.body, x.value, y.value,
                             aEventDetails, gLibrary);
}
