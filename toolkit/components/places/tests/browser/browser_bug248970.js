







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
    
    placeItemsCount = getPlacesItemsCount(aWindow);
    
    checkHistoryItems(aWindow);

    
    let bookmarkTitle = "title " + windowCount;
    let bookmarkKeyword = "keyword " + windowCount;
    let bookmarkUri = NetUtil.newURI("http://test-a-" + windowCount + ".com/");
    createBookmark(aWindow, bookmarkUri, bookmarkTitle, bookmarkKeyword);
    placeItemsCount++;
    windowCount++;
    ok(aWindow.PlacesUtils.bookmarks.isBookmarked(bookmarkUri),
       "Bookmark should be bookmarked, data should be retrievable");
    is(bookmarkKeyword, aWindow.PlacesUtils.bookmarks.getKeywordForURI(bookmarkUri),
       "Check bookmark uri keyword");
    is(getPlacesItemsCount(aWindow), placeItemsCount,
       "Check the new bookmark items count");
    is(isBookmarkAltered(aWindow), false, "Check if bookmark has been visited");

    aCallback();
  }

  clearHistory(function() {
    
    placeItemsCount = getPlacesItemsCount(window);
    
    is(PlacesUtils.history.hasHistoryEntries, false,
       "History database should be empty");
    
    fillHistoryVisitedURI(window);
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

function addVisit(aWin, aURI, aType) {
  aWin.PlacesUtils.history.addVisit(
    NetUtil.newURI(aURI), Date.now() * 1000, null, aType, false, 0);
}

function fillHistoryVisitedURI(aWin) {
  aWin.PlacesUtils.history.runInBatchMode({
    runBatched: function (aUserData) {
      addVisit(aWin, visitedURIs[0], PlacesUtils.history.TRANSITION_LINK);
      addVisit(aWin, visitedURIs[1], PlacesUtils.history.TRANSITION_TYPED);
      addVisit(aWin, visitedURIs[2], PlacesUtils.history.TRANSITION_BOOKMARK);
      addVisit(aWin, visitedURIs[3], PlacesUtils.history.TRANSITION_REDIRECT_PERMANENT);
      addVisit(aWin, visitedURIs[4], PlacesUtils.history.TRANSITION_REDIRECT_TEMPORARY);
      addVisit(aWin, visitedURIs[5], PlacesUtils.history.TRANSITION_EMBED);
      addVisit(aWin, visitedURIs[6], PlacesUtils.history.TRANSITION_FRAMED_LINK);
      addVisit(aWin, visitedURIs[7], PlacesUtils.history.TRANSITION_DOWNLOAD);
    }
  }, null);
}

function checkHistoryItems(aWin) {
  visitedURIs.forEach(function (visitedUri) {
    ok(aWin.PlacesUtils.bhistory.isVisited(NetUtil.newURI(visitedUri)), "");
    if (/embed/.test(visitedUri)) {
      is(!!pageInDatabase(visitedUri), false, "Check if URI is in database");
    } else {
      ok(!!pageInDatabase(visitedUri), "Check if URI is in database");
    }
  });
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
  options.resultType = options.RESULT_TYPE_VISIT;

  let query = aWin.PlacesUtils.history.getNewQuery();
  query.setFolders([aWin.PlacesUtils.bookmarks.bookmarksMenuFolder], 1);

  let root = aWin.PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  is(root.childCount, options.maxResults, "Check new bookmarks results");
  let node = root.getChild(0);
  root.containerOpen = false;

  return (node.accessCount != 0);
}
