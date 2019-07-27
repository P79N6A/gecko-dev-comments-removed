





const TOTAL_SITES = 20;

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  let now = Date.now() * 1000;

  for (let i = 0; i < TOTAL_SITES; i++) {
    let site = "http://www.test-" + i + ".com/";
    let testURI = uri(site);
    let testImageURI = uri(site + "blank.gif");
    let when = now + (i * TOTAL_SITES);
    yield promiseAddVisits([
      { uri: testURI, visitDate: when, transition: TRANSITION_TYPED },
      { uri: testImageURI, visitDate: ++when, transition: TRANSITION_EMBED },
      { uri: testImageURI, visitDate: ++when, transition: TRANSITION_FRAMED_LINK },
      { uri: testURI, visitDate: ++when, transition: TRANSITION_LINK },
    ]);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  options.includeHidden = true;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  
  do_check_eq(cc, 3 * TOTAL_SITES);
  for (let i = 0; i < TOTAL_SITES; i++) {
    let index = i * 3;
    let node = root.getChild(index);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
    node = root.getChild(++index);
    do_check_eq(node.uri, site + "blank.gif");
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
    node = root.getChild(++index);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  cc = root.childCount;
  
  do_check_eq(cc, 2 * TOTAL_SITES); 
  for (let i=0; i < TOTAL_SITES; i++) {
    let index = i * 2;
    let node = root.getChild(index);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
    node = root.getChild(++index);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.maxResults = 10;
  options.resultType = options.RESULTS_AS_URI;
  root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  cc = root.childCount;
  do_check_eq(cc, options.maxResults);
  for (let i=0; i < cc; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_URI;
  root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  cc = root.childCount;
  do_check_eq(cc, TOTAL_SITES);
  for (let i=0; i < 10; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;
});
