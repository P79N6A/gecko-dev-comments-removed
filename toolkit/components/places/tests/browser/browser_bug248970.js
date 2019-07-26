







let visitedURIs = [
  "http://www.test-link.com/",
  "http://www.test-typed.com/",
  "http://www.test-bookmark.com/",
  "http://www.test-redirect-permanent.com/",
  "http://www.test-redirect-temporary.com/",
  "http://www.test-embed.com/",
  "http://www.test-framed.com/",
  "http://www.test-download.com/"
];

function test() {
  waitForExplicitFinish();

  let windowsToClose = [];
  let windowCount = 0;
  let placeItemsCount = 0;

  registerCleanupFunction(function() {
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  function testOnWindow(aIsPrivate, aCallback) {
    whenNewWindowLoaded(aIsPrivate, function(win) {
      windowsToClose.push(win);
      checkPlaces(win, aIsPrivate, aCallback);
    });
  }

  function checkPlaces(aWindow, aIsPrivate, aCallback) {
    
    Task.spawn(checkHistoryItems).then(function() {
      
      placeItemsCount = getPlacesItemsCount(aWindow);
      
      let bookmarkTitle = "title " + windowCount;
      let bookmarkKeyword = "keyword " + windowCount;
      let bookmarkUri = NetUtil.newURI("http://test-a-" + windowCount + ".com/");
      createBookmark(aWindow, bookmarkUri, bookmarkTitle, bookmarkKeyword);
      placeItemsCount++;
      windowCount++;
      ok(PlacesUtils.bookmarks.isBookmarked(bookmarkUri),
         "Bookmark should be bookmarked, data should be retrievable");
      is(bookmarkKeyword, PlacesUtils.bookmarks.getKeywordForURI(bookmarkUri),
         "Check bookmark uri keyword");
      is(getPlacesItemsCount(aWindow), placeItemsCount,
         "Check the new bookmark items count");
      is(isBookmarkAltered(aWindow), false, "Check if bookmark has been visited");

      aCallback();
    });
  }

  clearHistory(function() {
    
    placeItemsCount = getPlacesItemsCount(window);
    
    is(PlacesUtils.history.hasHistoryEntries, false,
       "History database should be empty");
    
    fillHistoryVisitedURI(window, function() {
      placeItemsCount += 7;
      
      is(PlacesUtils.history.hasHistoryEntries, true,
         "History database should have entries");
      
      is(getPlacesItemsCount(window), placeItemsCount,
         "Check the total items count");
      
      testOnWindow(false, function() {
        testOnWindow(true, function() {
          testOnWindow(false, finish);
        });
      });
    });
  });
}

function whenNewWindowLoaded(aIsPrivate, aCallback) {
  let win = OpenBrowserWindow({private: aIsPrivate});
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    aCallback(win);
  }, false);
}

function clearHistory(aCallback) {
  Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
    aCallback();
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
  PlacesUtils.bhistory.removeAllPages();
}





function getPlacesItemsCount(aWin){
  
  let options = aWin.PlacesUtils.history.getNewQueryOptions();
  options.includeHidden = true;
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let root = aWin.PlacesUtils.history.executeQuery(
    aWin.PlacesUtils.history.getNewQuery(), options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  root.containerOpen = false;

  
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  let root = aWin.PlacesUtils.history.executeQuery(
    aWin.PlacesUtils.history.getNewQuery(), options).root;
  root.containerOpen = true;
  cc += root.childCount;
  root.containerOpen = false;

  return cc;
}

function fillHistoryVisitedURI(aWin, aCallback) {
  addVisits([
    {uri: NetUtil.newURI(visitedURIs[0]), transition: PlacesUtils.history.TRANSITION_LINK},
    {uri: NetUtil.newURI(visitedURIs[1]), transition: PlacesUtils.history.TRANSITION_TYPED},
    {uri: NetUtil.newURI(visitedURIs[2]), transition: PlacesUtils.history.TRANSITION_BOOKMARK},
    {uri: NetUtil.newURI(visitedURIs[3]), transition: PlacesUtils.history.TRANSITION_REDIRECT_PERMANENT},
    {uri: NetUtil.newURI(visitedURIs[4]), transition: PlacesUtils.history.TRANSITION_REDIRECT_TEMPORARY},
    {uri: NetUtil.newURI(visitedURIs[5]), transition: PlacesUtils.history.TRANSITION_EMBED},
    {uri: NetUtil.newURI(visitedURIs[6]), transition: PlacesUtils.history.TRANSITION_FRAMED_LINK},
    {uri: NetUtil.newURI(visitedURIs[7]), transition: PlacesUtils.history.TRANSITION_DOWNLOAD}],
    aWin, aCallback);
}

function checkHistoryItems() {
  for (let i = 0; i < visitedURIs.length; i++) {
    let visitedUri = visitedURIs[i];
    ok((yield promiseIsURIVisited(NetUtil.newURI(visitedUri))), "");
    if (/embed/.test(visitedUri)) {
      is(!!pageInDatabase(visitedUri), false, "Check if URI is in database");
    } else {
      ok(!!pageInDatabase(visitedUri), "Check if URI is in database");
    }
  }
}







function pageInDatabase(aURI) {
  let url = (aURI instanceof Ci.nsIURI ? aURI.spec : aURI);
  let stmt = DBConn().createStatement(
    "SELECT id FROM moz_places WHERE url = :url"
  );
  stmt.params.url = url;
  try {
    if (!stmt.executeStep())
      return 0;
    return stmt.getInt64(0);
  } finally {
    stmt.finalize();
  }
}












let gDBConn;
function DBConn(aForceNewConnection) {
  if (!aForceNewConnection) {
    let db =
      PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
    if (db.connectionReady)
      return db;
  }

  
  if (!gDBConn || aForceNewConnection) {
    let file = Services.dirsvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    let dbConn = gDBConn = Services.storage.openDatabase(file);

    
    Services.obs.addObserver(function DBCloseCallback(aSubject, aTopic, aData) {
      Services.obs.removeObserver(DBCloseCallback, aTopic);
      dbConn.asyncClose();
    }, "profile-before-change", false);
  }

  return gDBConn.connectionReady ? gDBConn : null;
};











function createBookmark(aWin, aURI, aTitle, aKeyword) {
  let bookmarkID = aWin.PlacesUtils.bookmarks.insertBookmark(
    aWin.PlacesUtils.bookmarksMenuFolderId, aURI,
    aWin.PlacesUtils.bookmarks.DEFAULT_INDEX, aTitle);
  aWin.PlacesUtils.bookmarks.setKeywordForBookmark(bookmarkID, aKeyword);
  return bookmarkID;
}








function isBookmarkAltered(aWin){
  let options = aWin.PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 1; 

  let query = aWin.PlacesUtils.history.getNewQuery();
  query.setFolders([aWin.PlacesUtils.bookmarks.bookmarksMenuFolder], 1);

  let root = aWin.PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  is(root.childCount, options.maxResults, "Check new bookmarks results");
  let node = root.getChild(0);
  root.containerOpen = false;

  return (node.accessCount != 0);
}
