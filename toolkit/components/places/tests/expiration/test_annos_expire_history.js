













let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);

function run_test() {
  run_next_test();
}

add_task(function test_annos_expire_history() {
  
  setInterval(3600); 

  
  setMaxPages(0);

  
  let now = getExpirablePRTime();
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://page_anno." + i + ".mozilla.org/");
    yield promiseAddVisits({ uri: pageURI, visitDate: now++ });
    as.setPageAnnotation(pageURI, "page_expire1", "test", 0, as.EXPIRE_WITH_HISTORY);
    as.setPageAnnotation(pageURI, "page_expire2", "test", 0, as.EXPIRE_WITH_HISTORY);
  }

  let pages = as.getPagesWithAnnotation("page_expire1");
  do_check_eq(pages.length, 5);
  pages = as.getPagesWithAnnotation("page_expire2");
  do_check_eq(pages.length, 5);

  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://item_anno." + i + ".mozilla.org/");
    
    yield promiseAddVisits({ uri: pageURI, visitDate: now++ });
    let id = bs.insertBookmark(bs.unfiledBookmarksFolder, pageURI,
                               bs.DEFAULT_INDEX, null);
    
    
    as.setPageAnnotation(pageURI, "item_persist1", "test", 0, as.EXPIRE_WITH_HISTORY);
    as.setPageAnnotation(pageURI, "item_persist2", "test", 0, as.EXPIRE_WITH_HISTORY);
  }

  let items = as.getPagesWithAnnotation("item_persist1");
  do_check_eq(items.length, 5);
  items = as.getPagesWithAnnotation("item_persist2");
  do_check_eq(items.length, 5);

  
  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://persist_page_anno." + i + ".mozilla.org/");
    yield promiseAddVisits({ uri: pageURI, visitDate: now++ });
    as.setPageAnnotation(pageURI, "page_persist1", "test", 0, as.EXPIRE_WITH_HISTORY);
    as.setPageAnnotation(pageURI, "page_persist2", "test", 0, as.EXPIRE_WITH_HISTORY);
  }

  pages = as.getPagesWithAnnotation("page_persist1");
  do_check_eq(pages.length, 5);
  pages = as.getPagesWithAnnotation("page_persist2");
  do_check_eq(pages.length, 5);

  
  yield promiseForceExpirationStep(10);

  pages = as.getPagesWithAnnotation("page_expire1");
  do_check_eq(pages.length, 0);
  pages = as.getPagesWithAnnotation("page_expire2");
  do_check_eq(pages.length, 0);
  items = as.getItemsWithAnnotation("item_persist1");
  do_check_eq(items.length, 0);
  items = as.getItemsWithAnnotation("item_persist2");
  do_check_eq(items.length, 0);
  pages = as.getPagesWithAnnotation("page_persist1");
  do_check_eq(pages.length, 5);
  pages = as.getPagesWithAnnotation("page_persist2");
  do_check_eq(pages.length, 5);
});
