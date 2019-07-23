







































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
}
catch(ex) {
  do_throw("Could not get bookmarks service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var mDBConn = histsvc.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
}
catch(ex) {
  do_throw("Could not get database connection\n");
}


function run_test() {
  var bookmarkedURI = uri("http://foo.bar");
  var keyword = "testkeyword";

  
  
  
  
  
  var bookmarkId = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,
                                        bookmarkedURI,
                                        bmsvc.DEFAULT_INDEX,
                                        "A bookmark");
  bmsvc.setKeywordForBookmark(bookmarkId, keyword);
  
  bmsvc.removeItem(bookmarkId);

  
  var sql = "SELECT id FROM moz_keywords WHERE keyword = ?1";
  var stmt = mDBConn.createStatement(sql);
  stmt.bindUTF8StringParameter(0, keyword);
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  
  
  
  
  var bookmarkId1 = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,
                                        bookmarkedURI,
                                        bmsvc.DEFAULT_INDEX,
                                        "A bookmark");
  bmsvc.setKeywordForBookmark(bookmarkId1, keyword);

  var bookmarkId2 = bmsvc.insertBookmark(bmsvc.toolbarFolder,
                                        bookmarkedURI,
                                        bmsvc.DEFAULT_INDEX,
                                        keyword);
  bmsvc.setKeywordForBookmark(bookmarkId2, keyword);

  
  bmsvc.removeItem(bookmarkId1);

  
  var sql = "SELECT id FROM moz_keywords WHERE keyword = ?1";
  var stmt = mDBConn.createStatement(sql);
  stmt.bindUTF8StringParameter(0, keyword);
  do_check_true(stmt.executeStep());
  stmt.finalize();
}
