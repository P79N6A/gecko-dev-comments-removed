



let secureURL = "https://example.com/browser/browser/base/content/test/general/browser_star_hsts.sjs";
let unsecureURL = "http://example.com/browser/browser/base/content/test/general/browser_star_hsts.sjs";

add_task(function* test_star_redirect() {
  registerCleanupFunction(function() {
    
    let sss = Cc["@mozilla.org/ssservice;1"]
                .getService(Ci.nsISiteSecurityService);
    sss.removeState(Ci.nsISiteSecurityService.HEADER_HSTS,
                    NetUtil.newURI("http://example.com/"), 0);
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
    gBrowser.removeCurrentTab();
  });

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  
  yield promiseTabLoadEvent(tab, secureURL, secureURL);
  
  yield promiseTabLoadEvent(tab, unsecureURL, secureURL);

  yield promiseStarState(BookmarkingUI.STATUS_UNSTARRED);

  let promiseBookmark = promiseOnItemAdded(gBrowser.currentURI);
  BookmarkingUI.star.click();
  
  
  yield promiseBookmark;

  is(BookmarkingUI.status, BookmarkingUI.STATUS_STARRED, "The star is starred");
});




function promiseStarState(aValue) {
  let deferred = Promise.defer();
  let expectedStatus = aValue ? BookmarkingUI.STATUS_STARRED
                              : BookmarkingUI.STATUS_UNSTARRED;
  (function checkState() {
    if (BookmarkingUI.status == BookmarkingUI.STATUS_UPDATING ||
        BookmarkingUI.status != expectedStatus) {
      info("Waiting for star button change.");
      setTimeout(checkState, 1000);
    } else {
      deferred.resolve();
    }
  })();
  return deferred.promise;
}












function promiseTabLoadEvent(aTab, aURL, aFinalURL)
{
  if (!aFinalURL)
    aFinalURL = aURL;
  let deferred = Promise.defer();
  info("Wait for load tab event");
  aTab.linkedBrowser.addEventListener("load", function load(event) {
    if (event.originalTarget != aTab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank" ||
        event.target.location.href != aFinalURL) {
      info("skipping spurious load event");
      return;
    }
    aTab.linkedBrowser.removeEventListener("load", load, true);
    info("Tab load event received");
    deferred.resolve();
  }, true, true);
  aTab.linkedBrowser.loadURI(aURL);
  return deferred.promise;
}




function promiseOnItemAdded(aExpectedURI) {
  let defer = Promise.defer();
  let bookmarksObserver = {
    onItemAdded: function (aItemId, aFolderId, aIndex, aItemType, aURI) {
      info("Added a bookmark to " + aURI.spec);
      PlacesUtils.bookmarks.removeObserver(bookmarksObserver);
      if (aURI.equals(aExpectedURI))
        defer.resolve();
      else
        defer.reject(new Error("Added an unexpected bookmark"));
    },
    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onItemRemoved: function () {},
    onItemChanged: function () {},
    onItemVisited: function () {},
    onItemMoved: function () {},
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsINavBookmarkObserver,
    ])
  };
  info("Waiting for a bookmark to be added");
  PlacesUtils.bookmarks.addObserver(bookmarksObserver, false);
  return defer.promise;
}
