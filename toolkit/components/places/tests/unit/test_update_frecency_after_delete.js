













add_test(function removed_bookmark()
{
  do_log_info("After removing bookmark, frecency of bookmark's URI should be " +
              "zero if URI is unvisited and no longer bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  promiseAsyncUpdates().then(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id);

    promiseAsyncUpdates().then(function ()
    {
      do_log_info("Unvisited URI no longer bookmarked => frecency should = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      promiseClearHistory().then(run_next_test);
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
  promiseAsyncUpdates().then(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    promiseAddVisits(TEST_URI).then(function () {
      PlacesUtils.bookmarks.removeItem(id);

      promiseAsyncUpdates().then(function ()
      {
        do_log_info("*Visited* URI no longer bookmarked => frecency should != 0");
        do_check_neq(frecencyForUrl(TEST_URI), 0);

        remove_all_bookmarks();
        promiseClearHistory().then(run_next_test);
      });
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
  promiseAsyncUpdates().then(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id1);

    promiseAsyncUpdates().then(function ()
    {
      do_log_info("URI still bookmarked => frecency should != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      promiseClearHistory().then(run_next_test);
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
  promiseAsyncUpdates().then(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    promiseAddVisits(TEST_URI).then(function () {
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

      promiseAsyncUpdates().then(function ()
      {
        do_log_info("*Visited* URI no longer bookmarked => frecency should != 0");
        do_check_neq(frecencyForUrl(TEST_URI), 0);

        remove_all_bookmarks();
        promiseClearHistory().then(run_next_test);
      });
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
  promiseAsyncUpdates().then(function ()
  {
    do_log_info("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    promiseAsyncUpdates().then(function ()
    {
      
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      promiseClearHistory().then(run_next_test);
    });
  });
});



function run_test()
{
  run_next_test();
}
