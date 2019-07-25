





































let bs = PlacesUtils.bookmarks;
let db = DBConn();

function check_keyword(aItemId, aExpectedBookmarkKeyword, aExpectedURIKeyword) {
  
  if (aExpectedURIKeyword)
    aExpectedURIKeyword = aExpectedURIKeyword.toLowerCase();
  if (aExpectedBookmarkKeyword)
    aExpectedBookmarkKeyword = aExpectedBookmarkKeyword.toLowerCase();

  if (aItemId) {
    print("Check keyword for bookmark");
    do_check_eq(bs.getKeywordForBookmark(aItemId), aExpectedBookmarkKeyword);

    print("Check keyword for uri");
    let uri = bs.getBookmarkURI(aItemId);
    do_check_eq(bs.getKeywordForURI(uri), aExpectedURIKeyword);

    print("Check uri for keyword");
    
    if (aExpectedURIKeyword) {
      do_check_true(/http:\/\/test[0-9]\.mozilla\.org/.test(bs.getURIForKeyword(aExpectedURIKeyword).spec));
      
      do_check_true(/http:\/\/test[0-9]\.mozilla\.org/.test(bs.getURIForKeyword(aExpectedURIKeyword.toUpperCase()).spec));
    }
  }
  else {
    stmt = db.createStatement(
      "SELECT id FROM moz_keywords WHERE keyword = :keyword"
    );
    stmt.params.keyword = aExpectedBookmarkKeyword;
    try {
      do_check_false(stmt.executeStep());
    } finally {
      stmt.finalize();
    }
  }

  print("Check there are no orphan database entries");
  let stmt = db.createStatement(
    "SELECT b.id FROM moz_bookmarks b "
  + "LEFT JOIN moz_keywords k ON b.keyword_id = k.id "
  + "WHERE keyword_id NOTNULL AND k.id ISNULL"
  );
  try {
    do_check_false(stmt.executeStep());
  } finally {
    stmt.finalize();
  }
}

function run_test() {
  print("Check that leyword does not exist");
  do_check_eq(bs.getURIForKeyword("keyword"), null);

  let folderId = bs.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                 "folder", bs.DEFAULT_INDEX);

  print("Add a bookmark with a keyword");
  let itemId1 = bs.insertBookmark(folderId,
                                  uri("http://test1.mozilla.org/"),
                                  bs.DEFAULT_INDEX,
                                  "test1");
  check_keyword(itemId1, null, null);
  bs.setKeywordForBookmark(itemId1, "keyword");
  check_keyword(itemId1, "keyword", "keyword");
  
  check_keyword(itemId1, "kEyWoRd", "kEyWoRd");

  print("Add another bookmark with the same uri, should not inherit keyword.");
  let itemId1_bis = bs.insertBookmark(folderId,
                                      uri("http://test1.mozilla.org/"),
                                      bs.DEFAULT_INDEX,
                                      "test1_bis");

  check_keyword(itemId1_bis, null, "keyword");
  
  check_keyword(itemId1_bis, null, "kEyWoRd");

  print("Set same keyword on another bookmark with a different uri.");
  let itemId2 = bs.insertBookmark(folderId,
                                  uri("http://test2.mozilla.org/"),
                                  bs.DEFAULT_INDEX,
                                  "test2");
  check_keyword(itemId2, null, null);
  bs.setKeywordForBookmark(itemId2, "kEyWoRd");
  check_keyword(itemId1, "kEyWoRd", "kEyWoRd");
  
  check_keyword(itemId1, "keyword", "keyword");
  check_keyword(itemId1_bis, null, "keyword");
  check_keyword(itemId2, "keyword", "keyword");

  print("Remove a bookmark with a keyword, it should not be removed from others");
  bs.removeItem(itemId2);
  check_keyword(itemId1, "keyword", "keyword");

  print("Remove a folder containing bookmarks with keywords");
  
  bs.removeItem(folderId);
  check_keyword(null, "keyword");
}

