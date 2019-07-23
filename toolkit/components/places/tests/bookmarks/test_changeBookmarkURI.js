







































var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);












function checkUris(aBookmarkId, aBookmarkedUri, aUnbookmarkedUri)
{
  
  var uri = bmsvc.getBookmarkedURIFor(aBookmarkedUri);
  do_check_neq(uri, null);
  do_check_true(uri.equals(aBookmarkedUri));

  
  do_check_true(bmsvc.isBookmarked(aBookmarkedUri));

  
  do_check_true(bmsvc.getBookmarkURI(aBookmarkId).equals(aBookmarkedUri));

  
  uri = bmsvc.getBookmarkedURIFor(aUnbookmarkedUri);
  do_check_eq(uri, null);

  
  do_check_false(bmsvc.isBookmarked(aUnbookmarkedUri));
}


function run_test() {
  
  var folderId = bmsvc.createFolder(bmsvc.toolbarFolder,
                                    "test",
                                    bmsvc.DEFAULT_INDEX);

  
  var uri1 = uri("http://www.dogs.com");
  var uri2 = uri("http://www.cats.com");

  
  var bookmarkId = bmsvc.insertBookmark(folderId,
                                        uri1,
                                        bmsvc.DEFAULT_INDEX,
                                        "Dogs");

  
  checkUris(bookmarkId, uri1, uri2);

  
  bmsvc.changeBookmarkURI(bookmarkId, uri2);

  
  checkUris(bookmarkId, uri2, uri1);
}
