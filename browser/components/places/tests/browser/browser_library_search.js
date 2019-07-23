






























































var testCases = [

  
  function () {
    var defScope = getDefaultScope(PlacesUIUtils.allBookmarksFolderId);
    search(PlacesUIUtils.allBookmarksFolderId, "dummy search", defScope);
    is(selectScope("scopeBarFolder"), false,
       "Folder scope should be disabled for All Bookmarks");
    resetSearch(defScope);
  },

  
  function () {
    defScope = getDefaultScope(PlacesUIUtils.leftPaneQueries["History"]);
    search(PlacesUIUtils.leftPaneQueries["History"], "dummy search", defScope);
    is(selectScope("scopeBarFolder"), false,
       "Folder scope should be disabled for History");
    resetSearch(defScope);
  },

  
  function () {
    defScope = getDefaultScope(bmsvc.toolbarFolder);
    search(bmsvc.toolbarFolder, "dummy search", defScope);
    is(selectScope("scopeBarFolder"), true,
       "Folder scope should be enabled for toolbar folder");
    
    
    resetSearch("scopeBarFolder");
    search(bmsvc.toolbarFolder, "dummy search", "scopeBarFolder");
  },

  
  function () {
    var folderId = bmsvc.createFolder(bmsvc.toolbarFolder,
                                      "dummy folder",
                                      bmsvc.DEFAULT_INDEX);
    defScope = getDefaultScope(folderId);
    search(folderId, "dummy search", defScope);
    is(selectScope("scopeBarFolder"), true,
       "Folder scope should be enabled for regular subfolder");
    
    
    resetSearch("scopeBarFolder");
    search(folderId, "dummy search", "scopeBarFolder");
    bmsvc.removeItem(folderId);
  },
];



const bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
const histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
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
      
      
      var query = queryStringToQuery(contentTree.getResult().root.uri);
      is(query.getFolders({}, {})[0], aFolderId,
         "Content tree's folder should be what was selected in the left pane");
    }
  }

  
  
  var searchBox = doc.getElementById("searchFilter");
  searchBox.value = aSearchStr;
  libraryWin.PlacesSearchBox.search(searchBox.value);
  query = queryStringToQuery(contentTree.getResult().root.uri);
  if (aSearchStr) {
    is(query.searchTerms, aSearchStr,
       "Content tree's searchTerms should be text in search box");
    is(doc.getElementById("searchModifiers").hidden, false,
       "Scope bar should not be hidden after searching");
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
  finish();
}



function test() {
  waitForExplicitFinish();

  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  var windowObserver = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic === "domwindowopened") {
        ww.unregisterNotification(this);
        var win = aSubject.QueryInterface(Ci.nsIDOMWindow);
        win.addEventListener("load", function onLoad(event) {
          win.removeEventListener("load", onLoad, false);
          executeSoon(function () testHelper(win));
        }, false);
      }
    }
  };

  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}
