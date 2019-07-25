






































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"]
                .getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"]
                  .getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var annosvc = Cc["@mozilla.org/browser/annotation-service;1"]
                  .getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
}








function uri_in_db(aURI) {
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI
  var query = histsvc.getNewQuery();
  query.uri = aURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
}

const TOTAL_SITES = 20;


function run_test() {

  
  try {
    for (var i = 0; i < TOTAL_SITES; i++) {
      let site = "http://www.test-" + i + ".com/";
      let testURI = uri(site);
      let when = Date.now() * 1000 + (i * TOTAL_SITES);
      histsvc.addVisit(testURI, when, null, histsvc.TRANSITION_TYPED, false, 0);
    }
    for (var i = 0; i < TOTAL_SITES; i++) {
      let site = "http://www.test.com/" + i + "/";
      let testURI = uri(site);
      let when = Date.now() * 1000 + (i * TOTAL_SITES);
      histsvc.addVisit(testURI, when, null, histsvc.TRANSITION_TYPED, false, 0);
    }
  } catch(ex) {
    do_throw("addPageWithDetails failed");
  }

  
  var testAnnoDeletedURI = uri("http://www.test.com/1/");
  var testAnnoDeletedName = "foo";
  var testAnnoDeletedValue = "bar";
  try {
    annosvc.setPageAnnotation(testAnnoDeletedURI, testAnnoDeletedName,
                              testAnnoDeletedValue, 0,
                              annosvc.EXPIRE_WITH_HISTORY);
  } catch(ex) {
    do_throw("setPageAnnotation failed");
  }

  
  var testAnnoRetainedURI = uri("http://www.test-1.com/");
  var testAnnoRetainedName = "foo";
  var testAnnoRetainedValue = "bar";
  try {
    annosvc.setPageAnnotation(testAnnoRetainedURI, testAnnoRetainedName,
                              testAnnoRetainedValue, 0,
                              annosvc.EXPIRE_WITH_HISTORY);
  } catch(ex) {
    do_throw("setPageAnnotation failed");
  }

  
  bhist.removePagesFromHost("www.test.com", false);

  
  for (var i = 0; i < TOTAL_SITES; i++) {
    let site = "http://www.test.com/" + i + "/";
    let testURI = uri(site);
    do_check_false(uri_in_db(testURI));
  }

  
  for (var i = 0; i < TOTAL_SITES; i++) {
    let site = "http://www.test-" + i + ".com/";
    let testURI = uri(site);
    do_check_true(uri_in_db(testURI));
  }

  
  try {
    annosvc.getPageAnnotation(testAnnoDeletedURI, testAnnoName);
    do_throw("fetching page-annotation that doesn't exist, should've thrown");
  } catch(ex) {}

  
  try {
    var annoVal = annosvc.getPageAnnotation(testAnnoRetainedURI,
                                            testAnnoRetainedName);
  } catch(ex) {
    do_throw("The annotation has been removed erroneously");
  }
  do_check_eq(annoVal, testAnnoRetainedValue);

}
