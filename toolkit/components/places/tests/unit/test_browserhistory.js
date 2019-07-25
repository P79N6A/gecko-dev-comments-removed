








































const TEST_URL = "http://mozilla.com/";
const TEST_SUBDOMAIN_URL = "http://foobar.mozilla.com/";









function add_page(aURL, aTime)
{
  PlacesUtils.bhistory.addPageWithDetails(NetUtil.newURI(aURL), "test",
                                          aTime || Date.now() * 1000);
}

add_test(function test_addPageWithDetails()
{
  add_page(TEST_URL);
  do_check_eq(1, PlacesUtils.bhistory.count);
  run_next_test();
});

add_test(function test_removePage()
{
  PlacesUtils.bhistory.removePage(NetUtil.newURI(TEST_URL));
  do_check_eq(0, PlacesUtils.bhistory.count);
  run_next_test();
});

add_test(function test_removePages()
{
  let pages = [];
  for (let i = 0; i < 8; i++) {
    pages.push(NetUtil.newURI(TEST_URL + i));
    add_page(TEST_URL + i);
  }

  
  const ANNO_INDEX = 1;
  const ANNO_NAME = "testAnno";
  const ANNO_VALUE = "foo";
  const BOOKMARK_INDEX = 2;
  PlacesUtils.annotations.setPageAnnotation(pages[ANNO_INDEX],
                                            ANNO_NAME, ANNO_VALUE, 0,
                                            Ci.nsIAnnotationService.EXPIRE_NEVER);
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       pages[BOOKMARK_INDEX],
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "test bookmark");
  PlacesUtils.annotations.setPageAnnotation(pages[BOOKMARK_INDEX],
                                            ANNO_NAME, ANNO_VALUE, 0,
                                            Ci.nsIAnnotationService.EXPIRE_NEVER);

  PlacesUtils.bhistory.removePages(pages, pages.length);
  do_check_eq(0, PlacesUtils.bhistory.count);

  
  do_check_true(PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.unfiledBookmarksFolderId, 0) > 0);
  do_check_eq(PlacesUtils.annotations.getPageAnnotation(pages[BOOKMARK_INDEX], ANNO_NAME),
              ANNO_VALUE);

  
  try {
    PlacesUtils.annotations.getPageAnnotation(pages[ANNO_INDEX], ANNO_NAME);
    do_throw("did not expire expire_never anno on a not bookmarked item");
  } catch(ex) {}

  
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  waitForClearHistory(run_next_test);
});

add_test(function test_removePagesByTimeframe()
{
  let startDate = Date.now() * 1000;
  for (let i = 0; i < 10; i++) {
    add_page(TEST_URL + i, startDate + i);
  }

  
  PlacesUtils.bhistory.removePagesByTimeframe(startDate + 1, startDate + 8);

  
  for (let i = 0; i < 10; i++) {
    do_check_eq(page_in_database(NetUtil.newURI(TEST_URL + i)) == 0,
                i > 0 && i < 9);
  }

  
  PlacesUtils.bhistory.removePagesByTimeframe(startDate, startDate + 9);
  do_check_eq(0, PlacesUtils.bhistory.count);
  run_next_test();
});

add_test(function test_removePagesFromHost()
{
  add_page(TEST_URL);
  PlacesUtils.bhistory.removePagesFromHost("mozilla.com", true);
  do_check_eq(0, PlacesUtils.bhistory.count);
  run_next_test();
});

add_test(function test_removePagesFromHost_keepSubdomains()
{
  add_page(TEST_URL);
  add_page(TEST_SUBDOMAIN_URL);
  PlacesUtils.bhistory.removePagesFromHost("mozilla.com", false);
  do_check_eq(1, PlacesUtils.bhistory.count);
  run_next_test();
});

add_test(function test_removeAllPages()
{
  PlacesUtils.bhistory.removeAllPages();
  do_check_eq(0, PlacesUtils.bhistory.count);
  run_next_test();
});

function run_test()
{
  run_next_test();
}
