





function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  let count_visited_URIs = ["http://www.test-link.com/",
                            "http://www.test-typed.com/",
                            "http://www.test-bookmark.com/",
                            "http://www.test-redirect-permanent.com/",
                            "http://www.test-redirect-temporary.com/"];

  let notcount_visited_URIs = ["http://www.test-embed.com/",
                               "http://www.test-download.com/",
                               "http://www.test-framed.com/"];

  
  yield PlacesTestUtils.addVisits([
    { uri: uri("http://www.test-link.com/"),
      transition: TRANSITION_LINK },
    { uri: uri("http://www.test-typed.com/"),
      transition: TRANSITION_TYPED },
    { uri: uri("http://www.test-bookmark.com/"),
      transition: TRANSITION_BOOKMARK },
    { uri: uri("http://www.test-embed.com/"),
      transition: TRANSITION_EMBED },
    { uri: uri("http://www.test-framed.com/"),
      transition: TRANSITION_FRAMED_LINK },
    { uri: uri("http://www.test-redirect-permanent.com/"),
      transition: TRANSITION_REDIRECT_PERMANENT },
    { uri: uri("http://www.test-redirect-temporary.com/"),
      transition: TRANSITION_REDIRECT_TEMPORARY },
    { uri: uri("http://www.test-download.com/"),
      transition: TRANSITION_DOWNLOAD },
  ]);

  
  count_visited_URIs.forEach(function (visited_uri) {
    do_check_true(yield promiseIsURIVisited(uri(visited_uri)));
  });
  notcount_visited_URIs.forEach(function (visited_uri) {
    do_check_true(yield promiseIsURIVisited(uri(visited_uri)));
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
});
