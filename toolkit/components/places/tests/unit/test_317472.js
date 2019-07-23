






































const charset = "UTF-8";
const CHARSET_ANNO = "URIProperties/characterSet";


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
                getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get services\n");
}

const TEST_URI = uri("http://foo.com");
const TEST_BOOKMARKED_URI = uri("http://bar.com");

function run_test() {
  var now = Date.now();

  
  histsvc.addVisit(TEST_URI, now, null,
                   Ci.nsINavHistoryService.TRANSITION_TYPED,
                   false, 0);
  histsvc.addVisit(TEST_BOOKMARKED_URI, now, null,
                   Ci.nsINavHistoryService.TRANSITION_TYPED,
                   false, 0);

  
  var bm1 = bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder,
                                 TEST_BOOKMARKED_URI, bmsvc.DEFAULT_INDEX,
                                 TEST_BOOKMARKED_URI.spec);
  var bm2 = bmsvc.insertBookmark(bmsvc.toolbarFolder,
                                 TEST_BOOKMARKED_URI, bmsvc.DEFAULT_INDEX,
                                 TEST_BOOKMARKED_URI.spec);

  
  histsvc.setCharsetForURI(TEST_URI, charset);
  
  histsvc.setCharsetForURI(TEST_BOOKMARKED_URI, charset);

  
  do_check_eq(annosvc.getPageAnnotation(TEST_URI, CHARSET_ANNO), charset);

  
  do_check_eq(histsvc.getCharsetForURI(TEST_URI), charset);
  
  do_check_eq(histsvc.getCharsetForURI(TEST_BOOKMARKED_URI), charset);

  waitForClearHistory(continue_test);

  do_test_pending();
}

function continue_test() {
  
  do_check_neq(histsvc.getCharsetForURI(TEST_URI), charset);

  
  try {
    annosvc.getPageAnnotation(TEST_URI, CHARSET_ANNO);
    do_throw("Charset page annotation has not been removed correctly");
  } catch (e) {}

  
  do_check_eq(histsvc.getCharsetForURI(TEST_BOOKMARKED_URI), charset);

  
  histsvc.setCharsetForURI(TEST_BOOKMARKED_URI, "");
  do_check_neq(histsvc.getCharsetForURI(TEST_BOOKMARKED_URI), charset);

  do_test_finished();
}
