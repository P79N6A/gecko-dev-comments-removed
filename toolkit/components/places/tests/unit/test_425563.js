






































function add_visit(aURI, aType) {
  PlacesUtils.history.addVisit(uri(aURI), Date.now() * 1000, null, aType,
                                 false, 0);
}

function run_test() {
  let count_visited_URIs = ["http://www.test-link.com/",
                            "http://www.test-typed.com/",
                            "http://www.test-bookmark.com/",
                            "http://www.test-redirect-permanent.com/",
                            "http://www.test-redirect-temporary.com/"];

  let notcount_visited_URIs = ["http://www.test-embed.com/",
                               "http://www.test-download.com/",
                               "http://www.test-framed.com/"];

  
  add_visit("http://www.test-link.com/", TRANSITION_LINK);
  add_visit("http://www.test-typed.com/", TRANSITION_TYPED);
  add_visit("http://www.test-bookmark.com/", TRANSITION_BOOKMARK);
  add_visit("http://www.test-embed.com/", TRANSITION_EMBED);
  add_visit("http://www.test-framed.com/", TRANSITION_FRAMED_LINK);
  add_visit("http://www.test-redirect-permanent.com/", TRANSITION_REDIRECT_PERMANENT);
  add_visit("http://www.test-redirect-temporary.com/", TRANSITION_REDIRECT_TEMPORARY);
  add_visit("http://www.test-download.com/", TRANSITION_DOWNLOAD);

  
  count_visited_URIs.forEach(function (visited_uri) {
    do_check_eq(PlacesUtils.bhistory.isVisited(uri(visited_uri)), true);
  });
  notcount_visited_URIs.forEach(function (visited_uri) {
    do_check_eq(PlacesUtils.bhistory.isVisited(uri(visited_uri)), true);
  });

  
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  options.includeHidden = true;
  let query = PlacesUtils.history.getNewQuery();
  query.minVisits = 1;
  let root = PlacesUtils.history.executeQuery(query, options).root;

  root.containerOpen = true;
  let cc = root.childCount;
  do_check_eq(cc, count_visited_URIs.length);

  for (let i = 0; i < cc; i++) {
    let node = root.getChild(i);
    do_check_neq(count_visited_URIs.indexOf(node.uri), -1);
  }
  root.containerOpen = false;
}
