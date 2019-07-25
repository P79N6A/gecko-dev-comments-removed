





































 



let hs = PlacesUtils.history;
let bs = PlacesUtils.bookmarks;




function addFakeRedirect(aSourceVisitId, aDestVisitId, aRedirectType) {
  let stmt = DBConn().createStatement(
    "UPDATE moz_historyvisits " +
    "SET from_visit = :source, visit_type = :type " +
    "WHERE id = :dest");
  stmt.params.source = aSourceVisitId;
  stmt.params.dest = aDestVisitId;
  stmt.params.type = aRedirectType;
  try {
    stmt.executeStep();
  }
  finally {
    stmt.finalize();
  }
}

function run_test() {
  let now = Date.now() * 1000;
  const sourceURI = uri("http://test.mozilla.org/");
  
  let sourceVisitId = hs.addVisit(sourceURI,
                                  now,
                                  null,
                                  hs.TRANSITION_TYPED,
                                  false,
                                  0);
  do_check_eq(bs.getBookmarkedURIFor(sourceURI), null);

  let sourceItemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                       sourceURI,
                                       bs.DEFAULT_INDEX,
                                       "bookmark");
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));

  
  const permaURI = uri("http://perma.mozilla.org/");
  hs.addVisit(permaURI,
              now++,
              sourceURI,
              hs.TRANSITION_REDIRECT_PERMANENT,
              true,
              0);
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));
  do_check_true(bs.getBookmarkedURIFor(permaURI).equals(sourceURI));
  
  let permaItemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                      permaURI,
                                      bs.DEFAULT_INDEX,
                                      "bookmark");
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));
  do_check_true(bs.getBookmarkedURIFor(permaURI).equals(permaURI));
  
  bs.removeItem(permaItemId);
  
  do_check_true(bs.getBookmarkedURIFor(permaURI).equals(sourceURI));

  
  const tempURI = uri("http://perma.mozilla.org/");
  hs.addVisit(tempURI,
              now++,
              permaURI,
              hs.TRANSITION_REDIRECT_TEMPORARY,
              true,
              0);
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));
  do_check_true(bs.getBookmarkedURIFor(tempURI).equals(sourceURI));
  
  let tempItemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                     tempURI,
                                     bs.DEFAULT_INDEX,
                                     "bookmark");
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));
  do_check_true(bs.getBookmarkedURIFor(tempURI).equals(tempURI));

  
  bs.removeItem(tempItemId);
  
  do_check_true(bs.getBookmarkedURIFor(tempURI).equals(sourceURI));
  
  bs.removeItem(sourceItemId);
  do_check_eq(bs.getBookmarkedURIFor(tempURI), null);

  
  
  do_check_eq(bs.getBookmarkedURIFor(uri("http://does.not.exist/")), null);
  do_check_false(page_in_database("http://does.not.exist/"));
}
