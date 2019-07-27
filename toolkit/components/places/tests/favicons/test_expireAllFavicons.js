









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

  
  PlacesTestUtils.addVisits({ uri: TEST_PAGE_URI, transition: TRANSITION_TYPED }).then(
    function () {
      PlacesUtils.favicons.setAndFetchFaviconForPage(TEST_PAGE_URI,
                                                     SMALLPNG_DATA_URI, true,
                                                     PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);

      
      PlacesUtils.bookmarks.insertBookmark(
                            PlacesUtils.toolbarFolderId, BOOKMARKED_PAGE_URI,
                            PlacesUtils.bookmarks.DEFAULT_INDEX,
                            "Test bookmark");
      PlacesUtils.favicons.setAndFetchFaviconForPage(
        BOOKMARKED_PAGE_URI, SMALLPNG_DATA_URI, true,
          PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE,
          function () {
            
            PlacesUtils.favicons.expireAllFavicons();
          });
    });
});
