





































var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);

function testThrowsOnDeletedItemId(aItemId) {
  try {
    bmsvc.getItemGUID(aItemId);
    do_throw("getItemGUID should throw when called for a deleted item id");
  } catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_ILLEGAL_VALUE);
  }
}

function run_test() {
  var folderId = bmsvc.createFolder(bmsvc.placesRoot, "test folder",
                                    bmsvc.DEFAULT_INDEX);
  var bookmarkId = bmsvc.insertBookmark(folderId, uri("http://foo.tld.com/"),
                                        bmsvc.DEFAULT_INDEX, "a title");
  var separatorId = bmsvc.insertSeparator(folderId, bmsvc.DEFAULT_INDEX);

  bmsvc.removeItem(folderId);

  
  [folderId, bookmarkId, separatorId].forEach(testThrowsOnDeletedItemId);
}
