





























































const TEST_URL = "http://dummy.mozilla.org/";
const TEST_DOWNLOAD_URL = "http://dummy.mozilla.org/dummy.pdf";

let gLibrary;

let testCases = [
  function allBookmarksScope() {
    let defScope = getDefaultScope(PlacesUIUtils.allBookmarksFolderId);
    search(PlacesUIUtils.allBookmarksFolderId, "dummy", defScope);
    ok(!selectScope("scopeBarFolder"),
       "Folder scope should be disabled for All Bookmarks");
    ok(selectScope("scopeBarAll"),
       "Bookmarks scope should be enabled for All Bookmarks");
    resetSearch("scopeBarAll");
  },

  function historyScope() {
    let defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["History"]);
    search(PlacesUIUtils.leftPaneQueries["History"], "dummy", defScope);
    ok(!selectScope("scopeBarFolder"),
       "Folder scope should be disabled for History");
    ok(selectScope("scopeBarAll"),
       "Bookmarks scope should be enabled for History");
    resetSearch("scopeBarAll");
  },

  function downloadsScope() {
    let defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["Downloads"]);
    search(PlacesUIUtils.leftPaneQueries["Downloads"], "dummy", defScope);
    ok(!selectScope("scopeBarFolder"),
       "Folder scope should be disabled for Downloads");
    ok(!selectScope("scopeBarAll"),
       "Bookmarks scope should be disabled for Downloads");
    resetSearch(defScope);
  },

  function toolbarFolderScope() {
    let defScope = getDefaultScope(PlacesUtils.toolbarFolderId);
    search(PlacesUtils.toolbarFolderId, "dummy", defScope);
    ok(selectScope("scopeBarAll"),
       "Bookmarks scope should be enabled for toolbar folder");
    ok(selectScope("scopeBarFolder"),
       "Folder scope should be enabled for toolbar folder");
    
    
    resetSearch("scopeBarFolder");
    search(PlacesUtils.toolbarFolderId, "dummy", "scopeBarFolder");
  },

  function subFolderScope() {
    let folderId = PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId,
                                                      "dummy folder",
                                                      PlacesUtils.bookmarks.DEFAULT_INDEX);
    let defScope = getDefaultScope(folderId);
    search(folderId, "dummy", defScope);
    ok(selectScope("scopeBarAll"),
       "Bookmarks scope should be enabled for regularfolder");
    ok(selectScope("scopeBarFolder"),
       "Folder scope should be enabled for regular subfolder");
    
    
    resetSearch("scopeBarFolder");
    search(folderId, "dummy", "scopeBarFolder");
    PlacesUtils.bookmarks.removeItem(folderId);
  },
];










function getDefaultScope(aFolderId) {
  switch (aFolderId) {
    case PlacesUIUtils.leftPaneQueries["History"]:
      return "scopeBarHistory"
    case PlacesUIUtils.leftPaneQueries["Downloads"]:
      return "scopeBarDownloads";
    default:
      return "scopeBarAll";
  }
}






function getSelectedScopeButtonId() {
  let doc = gLibrary.document;
  let scopeButtons = doc.getElementById("organizerScopeBar").childNodes;
  for (let i = 0; i < scopeButtons.length; i++) {
    if (scopeButtons[i].checked)
      return scopeButtons[i].id;
  }
  return null;
}








function queryStringToQuery(aPlaceURI) {
  let queries = {};
  PlacesUtils.history.queryStringToQueries(aPlaceURI, queries, {}, {});
  return queries.value[0];
}








function resetSearch(aExpectedScopeButtonId) {
  search(null, "", aExpectedScopeButtonId);
}














function search(aFolderId, aSearchStr, aExpectedScopeButtonId) {
  let doc = gLibrary.document;
  let folderTree = doc.getElementById("placesList");
  let contentTree = doc.getElementById("placeContent");

  
  
  if (aFolderId) {
    folderTree.selectItems([aFolderId]);
    isnot(folderTree.selectedNode, null,
       "Sanity check: left pane tree should have selection after selecting!");

    
    
    if (aFolderId !== PlacesUIUtils.leftPaneQueries["History"] &&
        aFolderId !== PlacesUIUtils.leftPaneQueries["Downloads"]) {
      
      
      let query = queryStringToQuery(contentTree.result.root.uri);
      is(query.getFolders()[0], aFolderId,
         "Content tree's folder should be what was selected in the left pane");
    }
  }

  
  
  let searchBox = doc.getElementById("searchFilter");
  searchBox.value = aSearchStr;
  gLibrary.PlacesSearchBox.search(searchBox.value);
  let query = queryStringToQuery(contentTree.result.root.uri);
  if (aSearchStr) {
    is(query.searchTerms, aSearchStr,
       "Content tree's searchTerms should be text in search box");
    is(doc.getElementById("searchModifiers").hidden, false,
       "Scope bar should not be hidden after searching");

    let scopeButtonId = getSelectedScopeButtonId();
    if (scopeButtonId == "scopeBarDownloads" ||
        scopeButtonId == "scopeBarHistory" ||
        scopeButtonId == "scopeBarAll" ||
        aFolderId == PlacesUtils.unfiledBookmarksFolderId) {
      
      let url, count;
      if (scopeButtonId == "scopeBarDownloads") {
        url = TEST_DOWNLOAD_URL;
        count = 1;
      }
      else {
        url = TEST_URL;
        count = scopeButtonId == "scopeBarHistory" ? 2 : 1;
      }
      is(contentTree.view.rowCount, count, "Found correct number of results");

      let node = null;
      for (let i = 0; i < contentTree.view.rowCount; i++) {
        node = contentTree.view.nodeForTreeIndex(i);
        if (node.uri === url)
          break;
      }
      isnot(node, null, "At least the target node should be in the tree");
      is(node.uri, url, "URI of node should match target URL");
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
  let doc = gLibrary.document;
  let button = doc.getElementById(aScopeButtonId);
  isnot(button, null,
     "Sanity check: scope button with ID " + aScopeButtonId + " should exist");
  
  if (button.disabled || button.hidden)
    return false;
  button.click();
  return true;
}





function onLibraryAvailable() {
  testCases.forEach(function (aTest) aTest());

  gLibrary.close();
  gLibrary = null;

  
  PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  waitForClearHistory(finish);
}



function test() {
  waitForExplicitFinish();

  
  ok(PlacesUtils, "PlacesUtils in context");

  
  PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_URL),
                               Date.now() * 1000, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.history.addVisit(PlacesUtils._uri(TEST_DOWNLOAD_URL),
                               Date.now() * 1000, null,
                               PlacesUtils.history.TRANSITION_DOWNLOAD, false, 0);
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       PlacesUtils._uri(TEST_URL),
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "dummy");
  PlacesUtils.tagging.tagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);

  gLibrary = openLibrary(onLibraryAvailable);
}
