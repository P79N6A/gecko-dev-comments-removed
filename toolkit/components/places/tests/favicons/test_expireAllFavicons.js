









XPCOMUtils.defineLazyServiceGetter(this, "gHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");

const TEST_PAGE_URI = NetUtil.newURI("http://example.com/");
const BOOKMARKED_PAGE_URI = NetUtil.newURI("http://example.com/bookmarked");




function run_test() {
  run_next_test();
}

add_test(function test_expireAllFavicons() {
  
  Services.obs.addObserver(function EAF_observer(aSubject, aTopic, aData) {
    Services.obs.removeObserver(EAF_observer, aTopic);

    
    checkFaviconMissingForPage(TEST_PAGE_URI, function () {
      checkFaviconMissingForPage(BOOKMARKED_PAGE_URI, function () {
        run_next_test();
      });
    });
  }, PlacesUtils.TOPIC_FAVICONS_EXPIRED, false);

  
  gHistory.updatePlaces({
    uri: TEST_PAGE_URI,
    visits: [{
      transitionType: Ci.nsINavHistoryService.TRANSITION_TYPED,
      visitDate: Date.now() * 1000
    }]
  }, {
    handleError: function EAF_handleError(aResultCode, aPlaceInfo) {
      do_throw("Unexpected error: " + aResultCode);
    },
    handleResult: function EAF_handleResult(aPlaceInfo) {
      PlacesUtils.favicons.setAndFetchFaviconForPage(TEST_PAGE_URI,
                                                     SMALLPNG_DATA_URI, true);

      
      PlacesUtils.bookmarks.insertBookmark(
                            PlacesUtils.toolbarFolderId, BOOKMARKED_PAGE_URI,
                            PlacesUtils.bookmarks.DEFAULT_INDEX,
                            "Test bookmark");
      PlacesUtils.favicons.setAndFetchFaviconForPage(
        BOOKMARKED_PAGE_URI, SMALLPNG_DATA_URI, true,
        function EAF_onFaviconDataAvailable() {
          
          PlacesUtils.favicons.expireAllFavicons();
        });
    }
  });
});
