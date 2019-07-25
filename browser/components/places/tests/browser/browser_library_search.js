





























































const TEST_URL = "http://dummy.mozilla.org/";


var testCases = [

  
  function () {
    var defScope = getDefaultScope(PlacesUIUtils.allBookmarksFolderId);
    search(PlacesUIUtils.allBookmarksFolderId, "dummy", defScope);
    is(selectScope("scopeBarFolder"), false,
       "Folder scope should be disabled for All Bookmarks");
    resetSearch(defScope);
  },

  
  function () {
    var defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["History"]);
    search(PlacesUIUtils.leftPaneQueries["History"], "dummy", defScope);
    is(selectScope("scopeBarFolder"), false,
       "Folder scope should be disabled for History");
    resetSearch(defScope);
  },

  
  function () {
    var defScope = getDefaultScope(bmsvc.toolbarFolder);
    search(bmsvc.toolbarFolder, "dummy", defScope);
    is(selectScope("scopeBarFolder"), true,
       "Folder scope should be enabled for toolbar folder");
    
    
    resetSearch("scopeBarFolder");
    search(bmsvc.toolbarFolder, "dummy", "scopeBarFolder");
  },

  
  function () {
    var folderId = bmsvc.createFolder(bmsvc.toolbarFolder,
                                      "dummy folder",
                                      bmsvc.DEFAULT_INDEX);
    var defScope = getDefaultScope(folderId);
    search(folderId, "dummy", defScope);
    is(selectScope("scopeBarFolder"), true,
       "Folder scope should be enabled for regular subfolder");
    
    
    resetSearch("scopeBarFolder");
    search(folderId, "dummy", "scopeBarFolder");
    bmsvc.removeItem(folderId);
  },
];



var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
var libraryWin;










function getDefaultScope(aFolderId) {
  return aFolderId === PlacesUIUtils.leftPaneQueries["History"] ?
         "scopeBarHistory" :
         "scopeBarAll";
}






function getSelectedScopeButtonId() {
  var doc = libraryWin.document;
  var scopeButtons = doc.getElementById("organizerScopeBar").childNodes;
  for (let i = 0; i < scopeButtons.length; i++) {
    if (scopeButtons[i].checked)
      return scopeButtons[i].id;
  }
  return null;
}








function queryStringToQuery(aPlaceURI) {
  var queries = {};
  histsvc.queryStringToQueries(aPlaceURI, queries, {}, {});
  return queries.value[0];
}








function resetSearch(aExpectedScopeButtonId) {
  search(null, "", aExpectedScopeButtonId);
}














function search(aFolderId, aSearchStr, aExpectedScopeButtonId) {
  var doc = libraryWin.document;
  var folderTree = doc.getElementById("placesList");
  var contentTree = doc.getElementById("placeContent");

  
  
  if (aFolderId) {
    folderTree.selectItems([aFolderId]);
    isnot(folderTree.selectedNode, null,
       "Sanity check: left pane tree should have selection after selecting!");

    
    
    if (aFolderId !== PlacesUIUtils.leftPaneQueries["History"]) {
      
      
      var query = queryStringToQuery(contentTree.result.root.uri);
      is(query.getFolders()[0], aFolderId,
         "Content tree's folder should be what was selected in the left pane");
    }
  }

  
  
  var searchBox = doc.getElementById("searchFilter");
  searchBox.value = aSearchStr;
  libraryWin.PlacesSearchBox.search(searchBox.value);
  query = queryStringToQuery(contentTree.result.root.uri);
  if (aSearchStr) {
    is(query.searchTerms, aSearchStr,
       "Content tree's searchTerms should be text in search box");
    is(doc.getElementById("searchModifiers").hidden, false,
       "Scope bar should not be hidden after searching");
    if (getSelectedScopeButtonId() == "scopeBarHistory" ||
        getSelectedScopeButtonId() == "scopeBarAll" ||
        aFolderId == PlacesUtils.bookmarks.unfiledBookmarksFolder) {
      
      var node = null;
      for (var i = 0; i < contentTree.view.rowCount; i++) {
        node = contentTree.view.nodeForTreeIndex(i);
        if (node.uri === TEST_URL)
          break;
      }
      isnot(node, null, "At least the target node should be in the tree");
      is(node.uri, TEST_URL, "URI of node should match target URL");
    }
  }
  else {
    is(query.hasSearchTerms, false,
       "Content tree's searchTerms should not exist after search reset");
    ok(doc.getElementById("searchModifiers").hidden,
       "Scope bar should be hidden after search reset");
  }
  is(getSelectedScopeButtonId(), aExpectedScopeButtonId,
     "Proper scope button should be selected after searching or resetting");
}








function selectScope(aScopeButtonId) {
  var doc = libraryWin.document;
  var button = doc.getElementById(aScopeButtonId);
  isnot(button, null,
     "Sanity check: scope button with ID " + aScopeButtonId + "should exist");
  
  if (button.disabled || button.hidden)
    return false;
  button.click();
  return true;
}








function testHelper(aLibraryWin) {
  libraryWin = aLibraryWin;
  testCases.forEach(function (aTest) aTest());
  aLibraryWin.close();

  
  PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.bookmarks.unfiledBookmarksFolder);
  PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();

  finish();
}



function test() {
  waitForExplicitFinish();

  
  ok(PlacesUtils, "PlacesUtils in context");
  
  PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_URL),
                               Date.now() * 1000, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.unfiledBookmarksFolder,
                                       PlacesUtils._uri(TEST_URL),
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "dummy");
  PlacesUtils.tagging.tagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);

  openLibrary(testHelper);
}
