





 



let hs = PlacesUtils.history;
let bs = PlacesUtils.bookmarks;

function run_test() {
  run_next_test();
}

add_task(function test_getBookmarkedURIFor() {
  let now = Date.now() * 1000;
  const sourceURI = uri("http://test.mozilla.org/");
  
  yield PlacesTestUtils.addVisits({ uri: sourceURI, visitDate: now });
  do_check_eq(bs.getBookmarkedURIFor(sourceURI), null);

  let sourceItemId = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                       sourceURI,
                                       bs.DEFAULT_INDEX,
                                       "bookmark");
  do_check_true(bs.getBookmarkedURIFor(sourceURI).equals(sourceURI));

  
  const permaURI = uri("http://perma.mozilla.org/");
  yield PlacesTestUtils.addVisits({
    uri: permaURI,
    transition: TRANSITION_REDIRECT_PERMANENT,
    visitDate: now++,
    referrer: sourceURI
  });
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
  yield PlacesTestUtils.addVisits({
    uri: tempURI,
    transition: TRANSITION_REDIRECT_TEMPORARY,
    visitDate: now++,
    referrer: permaURI
  });

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
});
