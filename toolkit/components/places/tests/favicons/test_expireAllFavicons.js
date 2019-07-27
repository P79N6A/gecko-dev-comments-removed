



"use strict";

const TEST_PAGE_URI = NetUtil.newURI("http://example.com/");
const BOOKMARKED_PAGE_URI = NetUtil.newURI("http://example.com/bookmarked");

add_task(function* test_expireAllFavicons() {
  const {FAVICON_LOAD_NON_PRIVATE} = PlacesUtils.favicons;

  
  yield PlacesTestUtils.addVisits({ uri: TEST_PAGE_URI, transition: TRANSITION_TYPED });

  
  yield promiseSetIconForPage(TEST_PAGE_URI, SMALLPNG_DATA_URI);

  
  yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    url: BOOKMARKED_PAGE_URI,
    title: "Test bookmark"
  });

  
  yield promiseSetIconForPage(BOOKMARKED_PAGE_URI, SMALLPNG_DATA_URI);

  
  let promise = promiseTopicObserved(PlacesUtils.TOPIC_FAVICONS_EXPIRED);
  PlacesUtils.favicons.expireAllFavicons();
  yield promise;

  
  yield promiseFaviconMissingForPage(TEST_PAGE_URI);
  yield promiseFaviconMissingForPage(BOOKMARKED_PAGE_URI);
});

function run_test() {
  run_next_test();
}
