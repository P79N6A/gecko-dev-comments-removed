




"use strict";

const PAGE_URI = NetUtil.newURI("http://example.com/test_query_result");

add_task(function* test_query_result_favicon_changed_on_child() {
  
  let testBookmark = yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    title: "test_bookmark",
    url: PAGE_URI
  });

  
  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarksMenuFolderId,
                    PlacesUtils.toolbarFolderId], 2);

  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 10;
  options.excludeQueries = 1;
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;

  let result = PlacesUtils.history.executeQuery(query, options);
  let resultObserver = {
    __proto__: NavHistoryResultObserver.prototype,
    containerStateChanged(aContainerNode, aOldState, aNewState) {
      if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED) {
        
        
        
        
        PlacesUtils.favicons.setAndFetchFaviconForPage(PAGE_URI,
                                                       SMALLPNG_DATA_URI,
                                                       false,
                                                       PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
      }
    },
    nodeIconChanged(aNode) {
      do_throw("The icon should be set only for the page," +
               " not for the containing query.");
    }
  };
  result.addObserver(resultObserver, false);

  
  
  
  
  let promise = promiseFaviconChanged(PAGE_URI, SMALLPNG_DATA_URI);
  result.root.containerOpen = true;
  yield promise;

  
  
  
  
  yield PlacesTestUtils.promiseAsyncUpdates();
  result.removeObserver(resultObserver);

  
  result.root.containerOpen = false;
});

function run_test() {
  run_next_test();
}
