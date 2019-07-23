






































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

function run_test() {
  do_test_pending();
  var now = Date.now();
  var testURI = uri("http://foo.com");
  var testBookmarkedURI = uri("http://bar.com");

  
  histsvc.addVisit(testURI, now, null,
                   Ci.nsINavHistoryService.TRANSITION_TYPED,
                   false, 0);
  histsvc.addVisit(testBookmarkedURI, now, null,
                   Ci.nsINavHistoryService.TRANSITION_TYPED,
                   false, 0);

  
  var bm1 = bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder,
                                 testBookmarkedURI, bmsvc.DEFAULT_INDEX,
                                 testBookmarkedURI.spec);
  var bm2 = bmsvc.insertBookmark(bmsvc.toolbarFolder,
                                 testBookmarkedURI, bmsvc.DEFAULT_INDEX,
                                 testBookmarkedURI.spec);

  
  histsvc.setCharsetForURI(testURI, charset);
  
  histsvc.setCharsetForURI(testBookmarkedURI, charset);

  
  do_check_eq(annosvc.getPageAnnotation(testURI, CHARSET_ANNO), charset);

  
  do_check_eq(histsvc.getCharsetForURI(testURI), charset);
  
  do_check_eq(histsvc.getCharsetForURI(testBookmarkedURI), charset);

  
  bhist.removeAllPages();

  
  do_check_neq(histsvc.getCharsetForURI(testURI), charset);

  
  try {
    annosvc.getPageAnnotation(testURI, CHARSET_ANNO);
    do_throw("Charset page annotation has not been removed correctly");
  } catch (e) {}

  
  do_check_eq(histsvc.getCharsetForURI(testBookmarkedURI), charset);

  
  histsvc.setCharsetForURI(testBookmarkedURI, "");
  do_check_neq(histsvc.getCharsetForURI(testBookmarkedURI), charset);

  do_test_finished();
}
