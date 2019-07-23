






































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


function run_test() {
  var URI = uri("http://foo.tld.com/");
  var folderId = bmsvc.createFolder(bmsvc.placesRoot, "test folder",
                                    bmsvc.DEFAULT_INDEX);
  var bookmarkId = bmsvc.insertBookmark(folderId, URI, bmsvc.DEFAULT_INDEX, "a title");
  var separatorId = bmsvc.insertSeparator(folderId, bmsvc.DEFAULT_INDEX);

  
  do_check_eq(bmsvc.getItemGUID(bookmarkId), bmsvc.getItemGUID(bookmarkId));

  do_check_neq(bmsvc.getItemGUID(folderId), bmsvc.getItemGUID(bookmarkId));
  do_check_neq(bmsvc.getItemGUID(folderId), "");

  do_check_neq(bmsvc.getItemGUID(separatorId), bmsvc.getItemGUID(bookmarkId));
  do_check_neq(bmsvc.getItemGUID(separatorId), "");

  
  bmsvc.setItemGUID(bookmarkId, "asdf123");
  do_check_eq(bmsvc.getItemGUID(bookmarkId), "asdf123");

  bmsvc.setItemGUID(folderId, "123asdf");
  do_check_eq(bmsvc.getItemGUID(folderId), "123asdf");

  try {
    bmsvc.setItemGUID(bookmarkId, "123asdf"); 
  } catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_ILLEGAL_VALUE);
  }

  
  do_check_eq(bmsvc.getItemIdForGUID("asdf123"), bookmarkId);
  do_check_eq(bmsvc.getItemIdForGUID("123asdf"), folderId);

}
