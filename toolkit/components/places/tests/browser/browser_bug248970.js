







let visitedURIs = [
  "http://www.test-link.com/",
  "http://www.test-typed.com/",
  "http://www.test-bookmark.com/",
  "http://www.test-redirect-permanent.com/",
  "http://www.test-redirect-temporary.com/",
  "http://www.test-embed.com/",
  "http://www.test-framed.com/",
  "http://www.test-download.com/"
].map(NetUtil.newURI.bind(NetUtil));

add_task(function () {
  let windowsToClose = [];
  let placeItemsCount = 0;

  registerCleanupFunction(function() {
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  yield PlacesTestUtils.clearHistory();

   
  let bookmarksDeferred = Promise.defer();
  waitForCondition(() => {
    placeItemsCount = getPlacesItemsCount();
    return placeItemsCount > 0
  }, bookmarksDeferred.resolve, "Should have default bookmarks");
  yield bookmarksDeferred.promise;

  
  yield PlacesTestUtils.addVisits([
    { uri: visitedURIs[0], transition: TRANSITION_LINK },
    { uri: visitedURIs[1], transition: TRANSITION_TYPED },
    { uri: visitedURIs[2], transition: TRANSITION_BOOKMARK },
    { uri: visitedURIs[3], transition: TRANSITION_REDIRECT_PERMANENT },
    { uri: visitedURIs[4], transition: TRANSITION_REDIRECT_TEMPORARY },
    { uri: visitedURIs[5], transition: TRANSITION_EMBED },
    { uri: visitedURIs[6], transition: TRANSITION_FRAMED_LINK },
    { uri: visitedURIs[7], transition: TRANSITION_DOWNLOAD }
  ]);

  placeItemsCount += 7;
  
  is(getPlacesItemsCount(), placeItemsCount,
     "Check the total items count");

  function* testOnWindow(aIsPrivate, aCount) {
    let deferred = Promise.defer();
    whenNewWindowLoaded({ private: aIsPrivate }, deferred.resolve);
    let win = yield deferred.promise;
    windowsToClose.push(win);

    
    yield checkHistoryItems();

    
    let count = getPlacesItemsCount();

    
    let title = "title " + windowsToClose.length;
    let keyword = "keyword " + windowsToClose.length;
    let url = "http://test-a-" + windowsToClose.length + ".com/";

    yield PlacesUtils.bookmarks.insert({ url, title,
                                         parentGuid: PlacesUtils.bookmarks.menuGuid });
    yield PlacesUtils.keywords.insert({ url, keyword });
    count++;

    ok((yield PlacesUtils.bookmarks.fetch({ url })),
       "Bookmark should be bookmarked, data should be retrievable");
    is(getPlacesItemsCount(), count,
       "Check the new bookmark items count");
    is(isBookmarkAltered(), false, "Check if bookmark has been visited");
  }

  
  yield testOnWindow(false);
  yield testOnWindow(true);
  yield testOnWindow(false);
});





function getPlacesItemsCount() {
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.includeHidden = true;
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let root = PlacesUtils.history.executeQuery(
    PlacesUtils.history.getNewQuery(), options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  root.containerOpen = false;

  
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  root = PlacesUtils.history.executeQuery(
    PlacesUtils.history.getNewQuery(), options).root;
  root.containerOpen = true;
  cc += root.childCount;
  root.containerOpen = false;

  return cc;
}

function* checkHistoryItems() {
  for (let i = 0; i < visitedURIs.length; i++) {
    let visitedUri = visitedURIs[i];
    ok((yield promiseIsURIVisited(visitedUri)), "");
    if (/embed/.test(visitedUri.spec)) {
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








function isBookmarkAltered(){
  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 1; 

  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarks.bookmarksMenuFolder], 1);

  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  is(root.childCount, options.maxResults, "Check new bookmarks results");
  let node = root.getChild(0);
  root.containerOpen = false;

  return (node.accessCount != 0);
}
