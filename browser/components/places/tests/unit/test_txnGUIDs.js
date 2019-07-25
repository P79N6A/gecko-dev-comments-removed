











































function test_GUID_persistance(aTxn) {
  aTxn.doTransaction();
  waitForAsyncUpdates(function () {
    let itemId = PlacesUtils.bookmarks
                            .getIdForItemAt(PlacesUtils.unfiledBookmarksFolderId, 0);
    let GUID = PlacesUtils.bookmarks.getItemGUID(itemId);
    aTxn.undoTransaction();
    aTxn.redoTransaction();
    waitForAsyncUpdates(function() {
      let itemId = PlacesUtils.bookmarks
                              .getIdForItemAt(PlacesUtils.unfiledBookmarksFolderId, 0);
      do_check_eq(GUID, PlacesUtils.bookmarks.getItemGUID(itemId));
      aTxn.undoTransaction();
      waitForAsyncUpdates(run_next_test);
    });
  });
}

function run_test() {
  run_next_test();
}

add_test(function create_folder() {
  let createFolderTxn = new PlacesCreateFolderTransaction(
    "Test folder", PlacesUtils.unfiledBookmarksFolderId,
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  test_GUID_persistance(createFolderTxn);
});

add_test(function create_bookmark() {
  let createBookmarkTxn = new PlacesCreateBookmarkTransaction(
    NetUtil.newURI("http://www.example.com"), PlacesUtils.unfiledBookmarksFolderId,
    PlacesUtils.bookmarks.DEFAULT_INDEX, "Test bookmark"
  );
  test_GUID_persistance(createBookmarkTxn);
});
  
add_test(function create_separator() {
  let createSeparatorTxn = new PlacesCreateSeparatorTransaction(
    PlacesUtils.unfiledBookmarksFolderId, PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  test_GUID_persistance(createSeparatorTxn);
});

add_test(function tag_uri() {
  let tagURITxn = new PlacesTagURITransaction(
    NetUtil.newURI("http://www.example.com"), ["foo"]
  );
  test_GUID_persistance(tagURITxn);
});

add_test(function create_livemark() {
  let createLivemarkTxn = new PlacesCreateLivemarkTransaction(
    NetUtil.newURI("http://feeduri.com"), NetUtil.newURI("http://siteuri.com"),
    "Test livemark", PlacesUtils.unfiledBookmarksFolderId,
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  test_GUID_persistance(createLivemarkTxn);
});

