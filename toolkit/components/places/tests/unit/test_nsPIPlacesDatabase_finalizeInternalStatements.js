






































function run_test() {
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);

  
  
  let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  while (tm.mainThread.hasPendingEvents())
    tm.mainThread.processNextEvent(false);

  var mDBConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;

  hs.QueryInterface(Ci.nsPIPlacesDatabase).finalizeInternalStatements();
  mDBConn.close();
  do_check_false(mDBConn.connectionReady);
}
