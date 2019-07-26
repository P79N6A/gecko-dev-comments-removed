










const PAGE_URI = NetUtil.newURI("http://example.com/test_query_result");




function run_test()
{
  run_next_test();
}

add_test(function test_query_result_favicon_changed_on_child()
{
  
  let testBookmark = PlacesUtils.bookmarks.insertBookmark(
    PlacesUtils.bookmarksMenuFolderId,
    PAGE_URI,
    PlacesUtils.bookmarks.DEFAULT_INDEX,
    "test_bookmark");

  
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
    containerStateChanged: function QRFCOC_containerStateChanged(aContainerNode,
                                                                 aOldState,
                                                                 aNewState) {
      if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED) {
        
        
        
        
        PlacesUtils.favicons.setAndFetchFaviconForPage(PAGE_URI,
                                                       SMALLPNG_DATA_URI,
                                                       false,
                                                       PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
      }
    },
    nodeIconChanged: function QRFCOC_nodeIconChanged(aNode) {
      do_throw("The icon should be set only for the page," +
               " not for the containing query.");
    }
  };
  result.addObserver(resultObserver, false);

  waitForFaviconChanged(PAGE_URI, SMALLPNG_DATA_URI,
                        function QRFCOC_faviconChanged() {
    
    
    
    
    waitForAsyncUpdates(function QRFCOC_asyncUpdates() {
      do_execute_soon(function QRFCOC_soon() {
        result.removeObserver(resultObserver);

        
        result.root.containerOpen = false;
        run_next_test();
      });
    });
  });

  
  result.root.containerOpen = true;
});
