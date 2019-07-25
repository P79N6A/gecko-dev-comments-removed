












































let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);

function run_test() {
  do_test_pending();

  
  setInterval(3600); 

  
  let now = Date.now() * 1000;
  for (let i = 0; i < 10; i++) {
    let pageURI = uri("http://session_page_anno." + i + ".mozilla.org/");
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
    as.setPageAnnotation(pageURI, "test1", "test", 0, as.EXPIRE_SESSION);
    as.setPageAnnotation(pageURI, "test2", "test", 0, as.EXPIRE_SESSION);
  }

  
  for (let i = 0; i < 10; i++) {
    let pageURI = uri("http://session_item_anno." + i + ".mozilla.org/");
    let id = bs.insertBookmark(bs.unfiledBookmarksFolder, pageURI,
                               bs.DEFAULT_INDEX, null);
    as.setItemAnnotation(id, "test1", "test", 0, as.EXPIRE_SESSION);
    as.setItemAnnotation(id, "test2", "test", 0, as.EXPIRE_SESSION);
  }


  let pages = as.getPagesWithAnnotation("test1");
  do_check_eq(pages.length, 10);
  pages = as.getPagesWithAnnotation("test2");
  do_check_eq(pages.length, 10);
  let items = as.getItemsWithAnnotation("test1");
  do_check_eq(items.length, 10);
  items = as.getItemsWithAnnotation("test2");
  do_check_eq(items.length, 10);

  waitForAsyncUpdates(function() {
    let stmt = DBConn().createStatement(
      "SELECT id FROM moz_annos "
    + "UNION "
    + "SELECT id FROM moz_items_annos "
    );
    do_check_false(stmt.executeStep());
    stmt.finalize();

    do_test_finished();
  });

  shutdownPlaces();
}
