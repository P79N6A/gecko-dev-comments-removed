











































const TEST_URI = "http://test.com/";
const TEST_ICON_URI = "http://test.com/favicon.ico";

const TEST_BOOKMARK_URI = "http://bookmarked.test.com/";
const TEST_BOOKMARK_ICON_URI = "http://bookmarked.test.com/favicon.ico";

const TOPIC_ICONS_EXPIRATION_FINISHED = "places-favicons-expired";

add_test(function() {
    do_log_info("Test that expireAllFavicons works as expected.");

    let bmURI = NetUtil.newURI(TEST_BOOKMARK_URI);
    let vsURI = NetUtil.newURI(TEST_URI);

    Services.obs.addObserver(function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(arguments.callee, aTopic);

      
      try {
        PlacesUtils.favicons.getFaviconForPage(vsURI);
        do_throw("Visited page has still a favicon!");
      } catch (ex) {  }

      
      try {
        PlacesUtils.favicons.getFaviconForPage(bmURI);
        do_throw("Bookmarked page has still a favicon!");
      } catch (ex) {  }

      run_next_test();
    }, TOPIC_ICONS_EXPIRATION_FINISHED, false);

    
    PlacesUtils.bookmarks.insertBookmark(
      PlacesUtils.toolbarFolderId, bmURI,
      PlacesUtils.bookmarks.DEFAULT_INDEX, "Test bookmark"
    );

    
    PlacesUtils.favicons.setFaviconUrlForPage(
      bmURI, NetUtil.newURI(TEST_BOOKMARK_ICON_URI)
    );
    do_check_eq(PlacesUtils.favicons.getFaviconForPage(bmURI).spec,
                TEST_BOOKMARK_ICON_URI);

    
    let visitId = PlacesUtils.history.addVisit(
      vsURI, Date.now() * 1000, null,
      PlacesUtils.history.TRANSITION_TYPED, false, 0
    );

    
    PlacesUtils.favicons
               .setFaviconUrlForPage(vsURI, NetUtil.newURI(TEST_ICON_URI));
    do_check_eq(PlacesUtils.favicons.getFaviconForPage(vsURI).spec,
                TEST_ICON_URI);

    PlacesUtils.favicons.expireAllFavicons();
});

function run_test() {
  run_next_test();
}
