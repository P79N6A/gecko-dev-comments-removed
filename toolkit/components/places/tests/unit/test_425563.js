







































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);


function add_visit(aURI, aType) {
  var visitID = histsvc.addVisit(uri(aURI),
                                 Date.now() * 1000,
                                 null, 
                                 aType,
                                 false, 
                                 0);
  do_check_true(visitID > 0);
  return visitID;
}


function run_test() {
  var count_visited_URIs = ["http://www.test-link.com/",
                            "http://www.test-typed.com/",
                            "http://www.test-bookmark.com/",
                            "http://www.test-redirect-permanent.com/",
                            "http://www.test-redirect-temporary.com/"];

  var notcount_visited_URIs = ["http://www.test-embed.com/",
                               "http://www.test-download.com/",
                               "http://www.test-framed.com/"];

  
  add_visit("http://www.test-link.com/", histsvc.TRANSITION_LINK);
  add_visit("http://www.test-typed.com/", histsvc.TRANSITION_TYPED);
  add_visit("http://www.test-bookmark.com/", histsvc.TRANSITION_BOOKMARK);
  add_visit("http://www.test-embed.com/", histsvc.TRANSITION_EMBED);
  add_visit("http://www.test-framed.com/", histsvc.TRANSITION_FRAMED_LINK);
  add_visit("http://www.test-redirect-permanent.com/", histsvc.TRANSITION_REDIRECT_PERMANENT);
  add_visit("http://www.test-redirect-temporary.com/", histsvc.TRANSITION_REDIRECT_TEMPORARY);
  add_visit("http://www.test-download.com/", histsvc.TRANSITION_DOWNLOAD);

  
  for each(var visited_uri in count_visited_URIs)
    do_check_eq(bhist.isVisited(uri(visited_uri)), true);
  for each(var visited_uri in notcount_visited_URIs)
    do_check_eq(bhist.isVisited(uri(visited_uri)), true);

  
  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  options.includeHidden = true;
  var query = histsvc.getNewQuery();
  query.minVisits = 1;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, count_visited_URIs.length);

  for (var i = 0; i < cc; i++) {
    var node = root.getChild(i);
    do_check_neq(count_visited_URIs.indexOf(node.uri), -1);
  }
  root.containerOpen = false;
}
