




function run_test()
{
  run_next_test()
}

add_task(function ()
{
  const TEST_URI = NetUtil.newURI("http://example.com/");
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                TEST_URI,
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "A title");
  yield PlacesTestUtils.promiseAsyncUpdates();
  do_check_true(frecencyForUrl(TEST_URI) > 0);

  
  
  PlacesUtils.bookmarks.removeItem(id);
  yield PlacesTestUtils.promiseAsyncUpdates();
  do_check_eq(frecencyForUrl(TEST_URI), 0);

  
  yield PlacesTestUtils.addVisits({ uri: TEST_URI });
  do_check_true(frecencyForUrl(TEST_URI) > 0);
});
