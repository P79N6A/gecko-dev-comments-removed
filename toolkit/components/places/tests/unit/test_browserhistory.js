









































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


function run_test() {
  var testURI = uri("http://mozilla.com");

  




  try {
    bhist.addPageWithDetails(testURI, "testURI", Date.now() * 1000);
  } catch(ex) {
    do_throw("addPageWithDetails failed");
  }

  



  do_check_eq("http://mozilla.com/", bhist.lastPageVisited);

  



  do_check_eq(1, bhist.count);

  


  try {
    bhist.removePage(testURI);
  } catch(ex) {
    do_throw("removePage failed");
  }
  do_check_eq(0, bhist.count);
  do_check_eq("", bhist.lastPageVisited);

  


  var deletedPages = [];
  deletedPages.push(uri("http://mirror1.mozilla.com"));
  deletedPages.push(uri("http://mirror2.mozilla.com"));
  deletedPages.push(uri("http://mirror3.mozilla.com"));
  deletedPages.push(uri("http://mirror4.mozilla.com"));
  deletedPages.push(uri("http://mirror5.mozilla.com"));
  deletedPages.push(uri("http://mirror6.mozilla.com"));
  deletedPages.push(uri("http://mirror7.mozilla.com"));
  deletedPages.push(uri("http://mirror8.mozilla.com"));

  try {
    for (var i = 0; i < deletedPages.length ; ++i)
      bhist.addPageWithDetails(deletedPages[i], "testURI" + (i+1), Date.now() * 1000);
  } catch(ex) {
    do_throw("addPageWithDetails failed");
  }

  
  var annoIndex = 1;
  var annoName = "testAnno";
  var annoValue = "foo";
  var bookmarkIndex = 2;
  var bookmarkName = "bar";  
  annosvc.setPageAnnotation(deletedPages[annoIndex], annoName, annoValue, 0,
                            Ci.nsIAnnotationService.EXPIRE_NEVER);
  var bookmark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,
      deletedPages[bookmarkIndex], bmsvc.DEFAULT_INDEX, bookmarkName);

  try {
    bhist.removePages(deletedPages, deletedPages.length, false);
  } catch(ex) {
    do_throw("removePages failed");
  }
  do_check_eq(0, bhist.count);
  do_check_eq("", bhist.lastPageVisited);
  
  do_check_eq(bmsvc.getBookmarkURI(bookmark).spec, deletedPages[bookmarkIndex].spec);
  do_check_eq(annosvc.getPageAnnotation(deletedPages[annoIndex], annoName), annoValue);
  
  annosvc.removePageAnnotation(deletedPages[annoIndex], annoName);
  bmsvc.removeItem(bookmark);
  bhist.removeAllPages();

  





  bhist.addPageWithDetails(testURI, "testURI", Date.now() * 1000);
  bhist.removePagesFromHost("mozilla.com", true);
  do_check_eq(0, bhist.count);

  
  bhist.addPageWithDetails(testURI, "testURI", Date.now() * 1000);
  bhist.addPageWithDetails(uri("http://foobar.mozilla.com"), "testURI2", Date.now() * 1000);
  bhist.removePagesFromHost("mozilla.com", false);
  do_check_eq(1, bhist.count);

  



  bhist.removeAllPages();
  do_check_eq(0, bhist.count);

  







  
  
  
  
}
