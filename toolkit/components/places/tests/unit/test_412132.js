












add_task(function changeuri_unvisited_bookmark()
{
  do_print("After changing URI of bookmark, frecency of bookmark's " +
           "original URI should be zero if original URI is unvisited and " +
           "no longer bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("Bookmarked => frecency of URI should be != 0");
  do_check_neq(frecencyForUrl(TEST_URI), 0);

  PlacesUtils.bookmarks.changeBookmarkURI(id, uri("http://example.com/2"));

  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("Unvisited URI no longer bookmarked => frecency should = 0");
  do_check_eq(frecencyForUrl(TEST_URI), 0);

  yield PlacesUtils.bookmarks.eraseEverything();
  yield PlacesTestUtils.clearHistory();
});

add_task(function changeuri_visited_bookmark()
{
  do_print("After changing URI of bookmark, frecency of bookmark's " +
           "original URI should not be zero if original URI is visited.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");

  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("Bookmarked => frecency of URI should be != 0");
  do_check_neq(frecencyForUrl(TEST_URI), 0);

  yield PlacesTestUtils.addVisits(TEST_URI);

  yield PlacesTestUtils.promiseAsyncUpdates();

  PlacesUtils.bookmarks.changeBookmarkURI(id, uri("http://example.com/2"));

  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("*Visited* URI no longer bookmarked => frecency should != 0");
  do_check_neq(frecencyForUrl(TEST_URI), 0);

  yield PlacesUtils.bookmarks.eraseEverything();
  yield PlacesTestUtils.clearHistory();
});

add_task(function changeuri_bookmark_still_bookmarked()
{
  do_print("After changing URI of bookmark, frecency of bookmark's " +
           "original URI should not be zero if original URI is still " +
           "bookmarked.");
  const TEST_URI = NetUtil.newURI("http://example.com/1");
  let id1 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                 TEST_URI,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark 1 title");
  let id2 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                 TEST_URI,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark 2 title");

  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("Bookmarked => frecency of URI should be != 0");
  do_check_neq(frecencyForUrl(TEST_URI), 0);

  PlacesUtils.bookmarks.changeBookmarkURI(id1, uri("http://example.com/2"));

  yield PlacesTestUtils.promiseAsyncUpdates();

  do_print("URI still bookmarked => frecency should != 0");
  do_check_neq(frecencyForUrl(TEST_URI), 0);

  yield PlacesUtils.bookmarks.eraseEverything();
  yield PlacesTestUtils.clearHistory();
});

add_task(function changeuri_nonexistent_bookmark()
{
  do_print("Changing the URI of a nonexistent bookmark should fail.");
  function tryChange(itemId)
  {
    try {
      PlacesUtils.bookmarks.changeBookmarkURI(itemId + 1, uri("http://example.com/2"));
      do_throw("Nonexistent bookmark should throw.");
    }
    catch (ex) {}
  }

  
  
  let stmt = DBConn().createStatement("SELECT MAX(id) FROM moz_bookmarks");
  stmt.executeStep();
  let maxId = stmt.getInt32(0);
  stmt.finalize();
  tryChange(maxId + 1);

  
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                uri("http://example.com/"),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "bookmark title");
  PlacesUtils.bookmarks.removeItem(id);
  tryChange(id);

  yield PlacesUtils.bookmarks.eraseEverything();
  yield PlacesTestUtils.clearHistory();
});



function run_test()
{
  run_next_test();
}
