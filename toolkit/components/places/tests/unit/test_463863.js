













































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);


function add_visit(aURI, aType) {
  var visitId = hs.addVisit(uri(aURI),
                            Date.now() * 1000,
                            null, 
                            aType,
                            false, 
                            0);
  do_check_true(visitId > 0);
  return visitId;
}

var  transitions = [ hs.TRANSITION_LINK,
                     hs.TRANSITION_TYPED,
                     hs.TRANSITION_BOOKMARK,
                     hs.TRANSITION_EMBED,
                     hs.TRANSITION_REDIRECT_PERMANENT,
                     hs.TRANSITION_REDIRECT_TEMPORARY,
                     hs.TRANSITION_DOWNLOAD ];

function runQuery(aResultType) {
  var options = hs.getNewQueryOptions();
  options.resultType = aResultType;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;
  var cc = root.childCount;
  do_check_eq(cc, transitions.length-1);

  for (var i = 0; i < cc; i++) {
    var node = root.getChild(i);
    
    do_check_neq(node.uri.substr(6,1), hs.TRANSITION_EMBED);
  }
  root.containerOpen = false;
}


function run_test() {
  
  transitions.forEach(
    function(transition) {
      add_visit("http://" + transition +".mozilla.org/", transition)
    });

  runQuery(Ci.nsINavHistoryQueryOptions.RESULTS_AS_VISIT);
  runQuery(Ci.nsINavHistoryQueryOptions.RESULTS_AS_URI);
}
