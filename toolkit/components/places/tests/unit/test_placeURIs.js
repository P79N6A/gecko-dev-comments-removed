







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function run_test() {
  
  

  const NHQO = Ci.nsINavHistoryQueryOptions;
  
  var query = histsvc.getNewQuery();
  query.setFolders([1],1);
  do_check_eq(histsvc.queriesToQueryString([query], 1, histsvc.getNewQueryOptions()),
              "place:folder=1");

  
  var options = histsvc.getNewQueryOptions();
  options.sortingAnnotation = "test anno";
  options.sortingMode = NHQO.SORT_BY_ANNOTATION_DESCENDING;
  var placeURI =
    "place:folder=1&sort=" + NHQO.SORT_BY_ANNOTATION_DESCENDING +
    "&sortingAnnotation=test%20anno";
  do_check_eq(histsvc.queriesToQueryString([query], 1, options),
              placeURI);
  var options = {};
  histsvc.queryStringToQueries(placeURI, { }, {}, options);
  do_check_eq(options.value.sortingAnnotation, "test anno");
  do_check_eq(options.value.sortingMode, NHQO.SORT_BY_ANNOTATION_DESCENDING);
}
