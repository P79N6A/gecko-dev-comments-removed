





const TOTAL_SITES = 20;

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  let places = [];
  for (let i = 0; i < TOTAL_SITES; i++) {
    for (let j = 0; j <= i; j++) {
      places.push({ uri: uri("http://www.test-" + i + ".com/"),
                    transition: TRANSITION_TYPED });
        
        
      places.push({ uri: uri("http://www.hidden.com/hidden.gif"),
                    transition: TRANSITION_EMBED });
      places.push({ uri: uri("http://www.alsohidden.com/hidden.gif"),
                    transition: TRANSITION_FRAMED_LINK });
    }
  }
  yield promiseAddVisits(places);

  
  
  
  
  
  
  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.maxResults = 10;
  options.resultType = options.RESULTS_AS_URI;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  do_check_eq(cc, options.maxResults);
  for (let i = 0; i < cc; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.resultType = options.RESULTS_AS_URI;
  root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  cc = root.childCount;
  do_check_eq(cc, TOTAL_SITES);
  for (let i = 0; i < 10; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, Ci.nsINavHistoryResultNode.RESULT_TYPE_URI);
  }
  root.containerOpen = false;
});
