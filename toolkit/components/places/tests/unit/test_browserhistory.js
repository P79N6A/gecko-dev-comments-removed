









































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].
                getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].
                 getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}








function uri_in_db(aURI) {
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI;
  var query = histsvc.getNewQuery();
  query.uri = aURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
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
      bhist.addPageWithDetails(deletedPages[i], "testURI" + (i+1),
                               Date.now() * 1000);
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
  annosvc.setPageAnnotation(deletedPages[bookmarkIndex],
                            annoName, annoValue, 0,
                            Ci.nsIAnnotationService.EXPIRE_NEVER);

  try {
    bhist.removePages(deletedPages, deletedPages.length, false);
  } catch(ex) {
    do_throw("removePages failed");
  }
  do_check_eq(0, bhist.count);
  do_check_eq("", bhist.lastPageVisited);
  
  do_check_eq(bmsvc.getBookmarkURI(bookmark).spec,
              deletedPages[bookmarkIndex].spec);
  do_check_eq(annosvc.getPageAnnotation(deletedPages[bookmarkIndex], annoName),
              annoValue);
  try {
    annosvc.getPageAnnotation(deletedPages[annoIndex], annoName);
    do_throw("did not expire expire_never anno on a not bookmarked item");
  } catch(ex) {}
  
  annosvc.removePageAnnotation(deletedPages[bookmarkIndex], annoName);
  bmsvc.removeItem(bookmark);
  waitForClearHistory(continue_test);
}

function continue_test() {
  



  
  var startDate = Date.now() * 1000;
  try {
    for (var i = 0; i < 10; ++i) {
      let testURI = uri("http://mirror" + i + ".mozilla.com");
      bhist.addPageWithDetails(testURI, "testURI" + i, startDate + i);
    }
  } catch(ex) {
    do_throw("addPageWithDetails failed");
  }
  
  bhist.removePagesByTimeframe(startDate+1, startDate+8);
  
  for (var i = 0; i < 10; ++i) {
    let testURI = uri("http://mirror" + i + ".mozilla.com");
    if (i > 0 && i < 9)
      do_check_false(uri_in_db(testURI));
    else
      do_check_true(uri_in_db(testURI));
  }
  
  bhist.removePagesByTimeframe(startDate, startDate+9);
  do_check_eq(0, bhist.count);

  





  bhist.addPageWithDetails(testURI, "testURI", Date.now() * 1000);
  bhist.removePagesFromHost("mozilla.com", true);
  do_check_eq(0, bhist.count);

  
  bhist.addPageWithDetails(testURI, "testURI", Date.now() * 1000);
  var testURI2 = uri("http://foobar.mozilla.com");
  bhist.addPageWithDetails(testURI2, "testURI2", Date.now() * 1000);
  bhist.removePagesFromHost("mozilla.com", false);
  do_check_eq(1, bhist.count);

  



  bhist.removeAllPages();
  do_check_eq(0, bhist.count);

  







  
  
  
  
}
