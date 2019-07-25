













































function add_visit(aURI, aDayOffset, aTransition) {
  PlacesUtils.history.addVisit(aURI, (Date.now() + aDayOffset*86400000) * 1000,
                               null, aTransition, false, 0);
}

function run_test() {
  add_visit(uri("http://mirror1.mozilla.com/a"), -1, TRANSITION_LINK);
  add_visit(uri("http://mirror2.mozilla.com/b"), -2, TRANSITION_LINK);
  add_visit(uri("http://mirror3.mozilla.com/c"), -4, TRANSITION_FRAMED_LINK);
  add_visit(uri("http://mirror1.google.com/b"), -1, TRANSITION_EMBED);
  add_visit(uri("http://mirror2.google.com/a"), -2, TRANSITION_LINK);
  add_visit(uri("http://mirror1.apache.org/b"), -3, TRANSITION_LINK);
  add_visit(uri("http://mirror2.apache.org/a"), -4, TRANSITION_FRAMED_LINK);

  let queries = [
    PlacesUtils.history.getNewQuery(),
    PlacesUtils.history.getNewQuery()
  ];
  queries[0].domain = "mozilla.com";
  queries[1].domain = "google.com";

  let root = PlacesUtils.history.executeQueries(
    queries, queries.length, PlacesUtils.history.getNewQueryOptions()
  ).root;
  root.containerOpen = true;
  let childCount = root.childCount;
  root.containerOpen = false;

  do_check_eq(childCount, 3);
}
