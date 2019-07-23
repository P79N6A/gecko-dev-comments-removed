














































let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);

function run_test() {
  
  setInterval(3600); 

  
  setMaxPages(0);

  
  let now = Date.now() * 1000;
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://page_anno." + i + ".mozilla.org/");
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
    as.setPageAnnotation(pageURI, "page_expire1", "test", 0, as.EXPIRE_WITH_HISTORY);
    as.setPageAnnotation(pageURI, "page_expire2", "test", 0, as.EXPIRE_WITH_HISTORY);
  }

  let pages = as.getPagesWithAnnotation("page_expire1");
  do_check_eq(pages.length, 5);
  pages = as.getPagesWithAnnotation("page_expire2");
  do_check_eq(pages.length, 5);

  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://item_anno." + i + ".mozilla.org/");
    
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
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
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
    as.setPageAnnotation(pageURI, "page_persist1", "test", 0, as.EXPIRE_WITH_HISTORY);
    as.setPageAnnotation(pageURI, "page_persist2", "test", 0, as.EXPIRE_WITH_HISTORY);
  }

  pages = as.getPagesWithAnnotation("page_persist1");
  do_check_eq(pages.length, 5);
  pages = as.getPagesWithAnnotation("page_persist2");
  do_check_eq(pages.length, 5);

  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      os.removeObserver(observer, TOPIC_EXPIRATION_FINISHED);

      let pages = as.getPagesWithAnnotation("page_expire1");
      do_check_eq(pages.length, 0);
      pages = as.getPagesWithAnnotation("page_expire2");
      do_check_eq(pages.length, 0);
      let items = as.getItemsWithAnnotation("item_persist1");
      do_check_eq(items.length, 0);
      items = as.getItemsWithAnnotation("item_persist2");
      do_check_eq(items.length, 0);
      pages = as.getPagesWithAnnotation("page_persist1");
      do_check_eq(pages.length, 5);
      pages = as.getPagesWithAnnotation("page_persist2");
      do_check_eq(pages.length, 5);

      do_test_finished();
    }
  };
  os.addObserver(observer, TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step(10);
  do_test_pending();
}
