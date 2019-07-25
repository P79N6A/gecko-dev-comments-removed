












































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);


function add_visit(aURI, aTime) {
  var visitId = hs.addVisit(uri(aURI),
                            aTime * 1000,
                            null, 
                            hs.TRANSITION_TYPED,
                            false, 
                            0);
  do_check_true(visitId > 0);
  return visitId;
}




add_test(function() {
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_QUERY;
  
  options.sortingMode = options.SORT_BY_NONE;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var dayContainer = root.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayContainer.containerOpen = true;

  var cc = dayContainer.childCount;
  do_check_eq(cc, pages.length);
  for (var i = 0; i < cc; i++) {
    var node = dayContainer.getChild(i);
    do_check_eq(pages[i], node.uri);
  }

  dayContainer.containerOpen = false;
  root.containerOpen = false;
});




add_test(function() {
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_QUERY;
  
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var dayContainer = root.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayContainer.containerOpen = true;

  var cc = dayContainer.childCount;
  do_check_eq(cc, pages.length);
  for (var i = 0; i < cc; i++) {
    var node = dayContainer.getChild(i);
    do_check_eq(pages[pages.length - i - 1], node.uri);
  }

  dayContainer.containerOpen = false;
  root.containerOpen = false;
});




add_test(function() {
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var dayContainer = root.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayContainer.containerOpen = true;
  var siteContainer = dayContainer.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(siteContainer.title, "a.mozilla.org");
  siteContainer.containerOpen = true;

  var cc = siteContainer.childCount;
  do_check_eq(cc, pages.length / 2);
  for (var i = 0; i < cc / 2; i++) {
    var node = siteContainer.getChild(i);
    do_check_eq(pages[i], node.uri);
  }

  siteContainer.containerOpen = false;
  dayContainer.containerOpen = false;
  root.containerOpen = false;
});


var pages = [
  "http://a.mozilla.org/1/",
  "http://a.mozilla.org/2/",
  "http://a.mozilla.org/3/",
  "http://a.mozilla.org/4/",
  "http://b.mozilla.org/5/",
  "http://b.mozilla.org/6/",
  "http://b.mozilla.org/7/",
  "http://b.mozilla.org/8/",
];


function run_test() {
  var noon = new Date();
  noon.setHours(12);

  
  pages.forEach(function(aPage) {
      add_visit(aPage, noon - (pages.length - pages.indexOf(aPage)) * 1000);
    });

  
  while (gTests.length)
    (gTests.shift())();

  hs.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
}
