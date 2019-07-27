


























const TEST_URL = "http://dummy.mozilla.org/";
const TEST_DOWNLOAD_URL = "http://dummy.mozilla.org/dummy.pdf";

let gLibrary;

let testCases = [
  function allBookmarksScope() {
    let defScope = getDefaultScope(PlacesUIUtils.allBookmarksFolderId);
    search(PlacesUIUtils.allBookmarksFolderId, "dummy", defScope);
  },

  function historyScope() {
    let defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["History"]);
    search(PlacesUIUtils.leftPaneQueries["History"], "dummy", defScope);
  },

  function downloadsScope() {
    let defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["Downloads"]);
    search(PlacesUIUtils.leftPaneQueries["Downloads"], "dummy", defScope);
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
  }
  else {
    is(query.hasSearchTerms, false,
       "Content tree's searchTerms should not exist after search reset");
  }
}





function onLibraryAvailable() {
  testCases.forEach(function (aTest) aTest());

  gLibrary.close();
  gLibrary = null;

  
  PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  PlacesTestUtils.clearHistory().then(finish);
}



function test() {
  waitForExplicitFinish();

  
  ok(PlacesUtils, "PlacesUtils in context");

  
  PlacesTestUtils.addVisits(
    [{ uri: PlacesUtils._uri(TEST_URL), visitDate: Date.now() * 1000,
       transition: PlacesUtils.history.TRANSITION_TYPED },
     { uri: PlacesUtils._uri(TEST_DOWNLOAD_URL), visitDate: Date.now() * 1000,
       transition: PlacesUtils.history.TRANSITION_DOWNLOAD }]
    ).then(() => {
      PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                           PlacesUtils._uri(TEST_URL),
                                           PlacesUtils.bookmarks.DEFAULT_INDEX,
                                           "dummy");
      PlacesUtils.tagging.tagURI(PlacesUtils._uri(TEST_URL), ["dummyTag"]);

      gLibrary = openLibrary(onLibraryAvailable);
    });
}
