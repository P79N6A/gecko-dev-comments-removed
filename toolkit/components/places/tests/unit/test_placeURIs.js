







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function run_test() {
  
  

  
  var query = histsvc.getNewQuery();
  query.setFolders([1],1);
  do_check_eq(histsvc.queriesToQueryString([query], 1, histsvc.getNewQueryOptions()),
              "place:folder=1");
}
