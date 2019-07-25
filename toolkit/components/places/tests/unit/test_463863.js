













































function add_visit(aURI, aType) {
  PlacesUtils.history.addVisit(uri(aURI), Date.now() * 1000, null, aType,
                            false, 0);
}

let transitions = [
  TRANSITION_LINK
, TRANSITION_TYPED
, TRANSITION_BOOKMARK
, TRANSITION_EMBED
, TRANSITION_FRAMED_LINK
, TRANSITION_REDIRECT_PERMANENT
, TRANSITION_REDIRECT_TEMPORARY
, TRANSITION_DOWNLOAD
];

function runQuery(aResultType) {
  let options = PlacesUtils.history.getNewQueryOptions();
  options.resultType = aResultType;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  do_check_eq(cc, transitions.length - 2);

  for (let i = 0; i < cc; i++) {
    let node = root.getChild(i);
    
    do_check_neq(node.uri.substr(6,1), TRANSITION_EMBED);
    do_check_neq(node.uri.substr(6,1), TRANSITION_FRAMED_LINK);
  }
  root.containerOpen = false;
}

function run_test() {
  
  transitions.forEach(function(transition) {
    add_visit("http://" + transition +".mozilla.org/", transition);
  });

  runQuery(Ci.nsINavHistoryQueryOptions.RESULTS_AS_VISIT);
  runQuery(Ci.nsINavHistoryQueryOptions.RESULTS_AS_URI);
}
