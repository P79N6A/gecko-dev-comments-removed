





const TEST_URI = NetUtil.newURI("http://mozilla.com/");
const TEST_SUBDOMAIN_URI = NetUtil.newURI("http://foobar.mozilla.com/");

add_task(function* test_addPage() {
  yield PlacesTestUtils.addVisits(TEST_URI);
  do_check_eq(1, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_removePage() {
  PlacesUtils.bhistory.removePage(TEST_URI);
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_removePages() {
  let pages = [];
  for (let i = 0; i < 8; i++) {
    pages.push(NetUtil.newURI(TEST_URI.spec + i));
  }

  yield PlacesTestUtils.addVisits(pages.map(function (uri) ({ uri: uri })));
  
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
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);

  
  do_check_true(PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.unfiledBookmarksFolderId, 0) > 0);
  do_check_eq(PlacesUtils.annotations.getPageAnnotation(pages[BOOKMARK_INDEX], ANNO_NAME),
              ANNO_VALUE);

  
  try {
    PlacesUtils.annotations.getPageAnnotation(pages[ANNO_INDEX], ANNO_NAME);
    do_throw("did not expire expire_never anno on a not bookmarked item");
  } catch(ex) {}

  
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  yield PlacesTestUtils.clearHistory();
});

add_task(function* test_removePagesByTimeframe() {
  let visits = [];
  let startDate = Date.now() * 1000;
  for (let i = 0; i < 10; i++) {
    visits.push({
      uri: NetUtil.newURI(TEST_URI.spec + i),
      visitDate: startDate + i
    });
  }

  yield PlacesTestUtils.addVisits(visits);

  
  PlacesUtils.bhistory.removePagesByTimeframe(startDate + 1, startDate + 8);

  
  for (let i = 0; i < 10; i++) {
    do_check_eq(page_in_database(NetUtil.newURI(TEST_URI.spec + i)) == 0,
                i > 0 && i < 9);
  }

  
  PlacesUtils.bhistory.removePagesByTimeframe(startDate, startDate + 9);
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_removePagesFromHost() {
  yield PlacesTestUtils.addVisits(TEST_URI);
  PlacesUtils.bhistory.removePagesFromHost("mozilla.com", true);
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_removePagesFromHost_keepSubdomains() {
  yield PlacesTestUtils.addVisits([{ uri: TEST_URI }, { uri: TEST_SUBDOMAIN_URI }]);
  PlacesUtils.bhistory.removePagesFromHost("mozilla.com", false);
  do_check_eq(1, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_history_clear() {
  yield PlacesTestUtils.clearHistory();
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);
});

add_task(function* test_getObservers() {
  
  yield PlacesTestUtils.addVisits(TEST_URI);
  do_check_eq(1, PlacesUtils.history.hasHistoryEntries);
  
  return new Promise((resolve, reject) => {
    DBConn().executeSimpleSQLAsync("DELETE FROM moz_historyvisits", {
      handleError: function(error) {
        reject(error);
      },
      handleResult: function(result) {
      },
      handleCompletion: function(result) {
        
        PlacesUtils.history.getObservers();
        do_check_eq(0, PlacesUtils.history.hasHistoryEntries);
        resolve();
      }
    });
  });
});

function run_test() {
  run_next_test();
}
