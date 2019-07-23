










































var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var txnsvc = Cc["@mozilla.org/browser/placesTransactionsService;1"].
             getService(Ci.nsIPlacesTransactionsService);

function test_GUID_persistance(aTxn) {
  aTxn.doTransaction();
  var itemId = bmsvc.getIdForItemAt(bmsvc.unfiledBookmarksFolder, 0);
  var GUID = bmsvc.getItemGUID(itemId);
  aTxn.undoTransaction();
  aTxn.redoTransaction();
  do_check_eq(GUID, bmsvc.getItemGUID(itemId));
  aTxn.undoTransaction();
}

function run_test() {
  
  var createFolderTxn = txnsvc.createFolder("Test folder",
                                            bmsvc.unfiledBookmarksFolder,
                                            bmsvc.DEFAULT_INDEX);
  test_GUID_persistance(createFolderTxn);

  
  var createBookmarkTxn = txnsvc.createItem(uri("http://www.example.com"),
                                            bmsvc.unfiledBookmarksFolder,
                                            bmsvc.DEFAULT_INDEX,
                                            "Test bookmark");
  test_GUID_persistance(createBookmarkTxn);

  
  var createSeparatorTxn = txnsvc.createSeparator(bmsvc.unfiledBookmarksFolder,
                                                  bmsvc.DEFAULT_INDEX);
  test_GUID_persistance(createFolderTxn);

  
  var createLivemarkTxn = txnsvc.createLivemark(uri("http://feeduri.com"),
                                               uri("http://siteuri.com"),
                                               "Test livemark",
                                               bmsvc.unfiledBookmarksFolder,
                                               bmsvc.DEFAULT_INDEX);
  test_GUID_persistance(createLivemarkTxn);

  
  var tagURITxn = txnsvc.tagURI(uri("http://www.example.com"), ["foo"]);
  test_GUID_persistance(tagURITxn);
}
