









































function add_visit(aURI, aWhen, aType) {
  PlacesUtils.history.addVisit(aURI, aWhen, null, aType, false, 0);
}

const TOTAL_SITES = 20;

function run_test() {
  let now = Date.now() * 1000;

  PlacesUtils.history.runInBatchMode({
    runBatched: function (aUserData) {
      for (let i=0; i < TOTAL_SITES; i++) {
        let site = "http://www.test-" + i + ".com/";
        let testURI = uri(site);
        let testImageURI = uri(site + "blank.gif");
        let when = now + (i * TOTAL_SITES);
        add_visit(testURI, when, PlacesUtils.history.TRANSITION_TYPED);
        add_visit(testImageURI, ++when, PlacesUtils.history.TRANSITION_EMBED);
        add_visit(testImageURI, ++when, PlacesUtils.history.TRANSITION_FRAMED_LINK);
        add_visit(testURI, ++when, PlacesUtils.history.TRANSITION_LINK);
      }
    }
  }, null);

  
  
  
  
  
  
  
  
  
  
  
  
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
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
    do_check_eq(node.uri, site + "blank.gif");
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  
  do_check_eq(cc, 2 * TOTAL_SITES); 
  for (let i=0; i < TOTAL_SITES; i++) {
    let index = i * 2;
    let node = root.getChild(index);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    node = root.getChild(++index);
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.maxResults = 10;
  options.resultType = options.RESULTS_AS_URI;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  do_check_eq(cc, options.maxResults);
  for (let i=0; i < cc; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_URI);
  }
  root.containerOpen = false;

  
  
  
  
  
  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_URI;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  do_check_eq(cc, TOTAL_SITES);
  for (let i=0; i < 10; i++) {
    let node = root.getChild(i);
    let site = "http://www.test-" + (TOTAL_SITES - 1 - i) + ".com/";
    do_check_eq(node.uri, site);
    do_check_eq(node.type, options.RESULTS_AS_URI);
  }
  root.containerOpen = false;
}
