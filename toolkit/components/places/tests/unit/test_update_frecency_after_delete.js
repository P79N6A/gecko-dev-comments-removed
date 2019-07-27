













add_test(function removed_bookmark()
{
  do_print("After removing bookmark, frecency of bookmark's URI should be " +
           "zero if URI is unvisited and no longer bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  PlacesTestUtils.promiseAsyncUpdates().then(() => {
    do_print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id);

    PlacesTestUtils.promiseAsyncUpdates().then(() => {
      do_print("Unvisited URI no longer bookmarked => frecency should = 0");
      do_check_eq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      PlacesTestUtils.clearHistory().then(run_next_test);
    });
  });
});

add_test(function removed_but_visited_bookmark()
{
  do_print("After removing bookmark, frecency of bookmark's URI should " +
           "not be zero if URI is visited.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  PlacesTestUtils.promiseAsyncUpdates().then(() => {
    do_print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesTestUtils.addVisits(TEST_URI).then(function () {
      PlacesUtils.bookmarks.removeItem(id);

      PlacesTestUtils.promiseAsyncUpdates().then(() => {
        do_print("*Visited* URI no longer bookmarked => frecency should != 0");
        do_check_neq(frecencyForUrl(TEST_URI), 0);

        remove_all_bookmarks();
        PlacesTestUtils.clearHistory().then(run_next_test);
      });
    });
  });
});

add_test(function remove_bookmark_still_bookmarked()
{
  do_print("After removing bookmark, frecency of bookmark's URI should " +
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
  PlacesTestUtils.promiseAsyncUpdates().then(() => {
    do_print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeItem(id1);

    PlacesTestUtils.promiseAsyncUpdates().then(() => {
      do_print("URI still bookmarked => frecency should != 0");
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      PlacesTestUtils.clearHistory().then(run_next_test);
    });
  });
});

add_test(function cleared_parent_of_visited_bookmark()
{
  do_print("After removing all children from bookmark's parent, frecency " +
           "of bookmark's URI should not be zero if URI is visited.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  PlacesTestUtils.promiseAsyncUpdates().then(() => {
    do_print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesTestUtils.addVisits(TEST_URI).then(function () {
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

      PlacesTestUtils.promiseAsyncUpdates().then(() => {
        do_print("*Visited* URI no longer bookmarked => frecency should != 0");
        do_check_neq(frecencyForUrl(TEST_URI), 0);

        remove_all_bookmarks();
        PlacesTestUtils.clearHistory().then(run_next_test);
      });
    });
  });
});

add_test(function cleared_parent_of_bookmark_still_bookmarked()
{
  do_print("After removing all children from bookmark's parent, frecency " +
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
  PlacesTestUtils.promiseAsyncUpdates().then(() => {
    do_print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(TEST_URI), 0);

    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

    PlacesTestUtils.promiseAsyncUpdates().then(() => {
      
      do_check_neq(frecencyForUrl(TEST_URI), 0);

      remove_all_bookmarks();
      PlacesTestUtils.clearHistory().then(run_next_test);
    });
  });
});



function run_test()
{
  run_next_test();
}
