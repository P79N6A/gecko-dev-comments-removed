














































add_test(function unvisited_bookmarked_livemarkItem()
{
  do_log_info("Frecency of unvisited, separately bookmarked livemark item's " +
              "URI should be zero after bookmark removed.");
  
  
  const TEST_URI = NetUtil.newURI("http://example.com/livemark-item");
  createLivemark(TEST_URI);
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id);

    waitForAsyncUpdates(function ()
    {
      do_log_info("URI's only bookmark is now unvisited livemark item => frecency = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function visited_bookmarked_livemarkItem()
{
  do_log_info("Frecency of visited, separately bookmarked livemark item's " +
              "URI should not be zero after bookmark removed.");
  
  
  const TEST_URI = NetUtil.newURI("http://example.com/livemark-item");
  createLivemark(TEST_URI);
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    visit(TEST_URI);
    PlacesUtils.bookmarks.removeItem(id);

    waitForAsyncUpdates(function ()
    {
      do_log_info("URI's only bookmark is now *visited* livemark item => frecency != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function removed_bookmark()
{
  do_log_info("After removing bookmark, frecency of bookmark's URI should be " +
              "zero if URI is unvisited and no longer bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id);

    waitForAsyncUpdates(function ()
    {
      do_log_info("Unvisited URI no longer bookmarked => frecency should = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function removed_but_visited_bookmark()
{
  do_log_info("After removing bookmark, frecency of bookmark's URI should " +
              "not be zero if URI is visited.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    visit(TEST_URI);
    PlacesUtils.bookmarks.removeItem(id);

    waitForAsyncUpdates(function ()
    {
      do_log_info("*Visited* URI no longer bookmarked => frecency should != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function remove_bookmark_still_bookmarked()
{
  do_log_info("After removing bookmark, frecency of bookmark's URI should ",
              "not be zero if URI is still bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id1 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                 TEST_URI,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark 1 title");
  let id2 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                 TEST_URI,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark 2 title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id1);

    waitForAsyncUpdates(function ()
    {
      do_log_info("URI still bookmarked => frecency should != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_unvisited_bookmark_to_livemarkItem()
{
  do_log_info("Frecency of unvisited, separately bookmarked livemark item's " +
              "URI should be zero after all children removed from bookmark's " +
              "parent.");
  
  
  const TEST_URI = NetUtil.newURI("http://example.com/livemark-item");
  createLivemark(TEST_URI);

  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    waitForAsyncUpdates(function ()
    {
      do_log_info("URI's only bookmark is now unvisited livemark item => frecency = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_visited_bookmark_to_livemarkItem()
{
  do_log_info("Frecency of visited, separately bookmarked livemark item's " +
              "URI should not be zero after all children removed from " +
              "bookmark's parent.");
  
  
  const TEST_URI = NetUtil.newURI("http://example.com/livemark-item");
  createLivemark(TEST_URI);
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    visit(TEST_URI);
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    waitForAsyncUpdates(function ()
    {
      do_log_info("URI's only bookmark is now *visited* livemark item => frecency != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_unvisited_unbookmarked_livemarkItem()
{
  do_log_info("After removing all children from bookmark's parent, frecency " +
              "of bookmark's URI should be zero if URI is unvisited and no " +
              "longer bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    waitForAsyncUpdates(function ()
    {
      do_log_info("Unvisited URI no longer bookmarked => frecency should = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_visited_bookmark()
{
  do_log_info("After removing all children from bookmark's parent, frecency " +
              "of bookmark's URI should not be zero if URI is visited.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    visit(TEST_URI);
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    waitForAsyncUpdates(function ()
    {
      do_log_info("*Visited* URI no longer bookmarked => frecency should != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_bookmark_still_bookmarked()
{
  do_log_info("After removing all children from bookmark's parent, frecency " +
              "of bookmark's URI should not be zero if URI is still " +
              "bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id1 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                                 TEST_URI,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark 1 title");

  let id2 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark 2 title");
  waitForAsyncUpdates(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    waitForAsyncUpdates(function ()
    {
      
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      waitForClearHistory(run_next_test);
    });
  });
});










function createLivemark(aChildURI)
{
  let livemarkId = PlacesUtils.livemarks.createLivemarkFolderOnly(
    PlacesUtils.unfiledBookmarksFolderId, "livemark title",
    uri("http://example.com/"), uri("http://example.com/rdf"), -1
  );
  return PlacesUtils.bookmarks.insertBookmark(livemarkId,
                                              aChildURI,
                                              PlacesUtils.bookmarks.DEFAULT_INDEX,
                                              "livemark item title");
}







function visit(aURI)
{
  PlacesUtils.history.addVisit(aURI, Date.now() * 1000, null,
                               PlacesUtils.history.TRANSITION_BOOKMARK,
                               false, 0);
}



function run_test()
{
  run_next_test();
}
