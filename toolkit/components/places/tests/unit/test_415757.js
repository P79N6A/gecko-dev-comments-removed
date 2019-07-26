












function uri_in_db(aURI) {
  var options = PlacesUtils.history.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI
  var query = PlacesUtils.history.getNewQuery();
  query.uri = aURI;
  var result = PlacesUtils.history.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;
  return (cc == 1);
}

const TOTAL_SITES = 20;


function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  
  for (var i = 0; i < TOTAL_SITES; i++) {
    let site = "http://www.test-" + i + ".com/";
    let testURI = uri(site);
    let when = Date.now() * 1000 + (i * TOTAL_SITES);
    yield promiseAddVisits({ uri: testURI, visitDate: when });
  }
  for (var i = 0; i < TOTAL_SITES; i++) {
    let site = "http://www.test.com/" + i + "/";
    let testURI = uri(site);
    let when = Date.now() * 1000 + (i * TOTAL_SITES);
    yield promiseAddVisits({ uri: testURI, visitDate: when });
  }

  
  var testAnnoDeletedURI = uri("http://www.test.com/1/");
  var testAnnoDeletedName = "foo";
  var testAnnoDeletedValue = "bar";
  PlacesUtils.annotations.setPageAnnotation(testAnnoDeletedURI,
                                            testAnnoDeletedName,
                                            testAnnoDeletedValue, 0,
                                            PlacesUtils.annotations.EXPIRE_WITH_HISTORY);

  
  var testAnnoRetainedURI = uri("http://www.test-1.com/");
  var testAnnoRetainedName = "foo";
  var testAnnoRetainedValue = "bar";
  PlacesUtils.annotations.setPageAnnotation(testAnnoRetainedURI,
                                            testAnnoRetainedName,
                                            testAnnoRetainedValue, 0,
                                            PlacesUtils.annotations.EXPIRE_WITH_HISTORY);

  
  PlacesUtils.history.removePagesFromHost("www.test.com", false);

  
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
    PlacesUtils.annotations.getPageAnnotation(testAnnoDeletedURI, testAnnoName);
    do_throw("fetching page-annotation that doesn't exist, should've thrown");
  } catch(ex) {}

  
  try {
    var annoVal = PlacesUtils.annotations.getPageAnnotation(testAnnoRetainedURI,
                                                            testAnnoRetainedName);
  } catch(ex) {
    do_throw("The annotation has been removed erroneously");
  }
  do_check_eq(annoVal, testAnnoRetainedValue);

});
