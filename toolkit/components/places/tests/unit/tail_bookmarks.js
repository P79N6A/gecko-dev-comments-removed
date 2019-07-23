











































flush_main_thread_events();



var os = Cc['@mozilla.org/observer-service;1'].
         getService(Ci.nsIObserverService);
os.notifyObservers(null, "quit-application-granted", null);
os.notifyObservers(null, "quit-application", null);


flush_main_thread_events();


var pip = Cc["@mozilla.org/browser/nav-history-service;1"].
          getService(Ci.nsINavHistoryService).
          QueryInterface(Ci.nsPIPlacesDatabase);
if (pip.DBConnection.connectionReady) {
  pip.commitPendingChanges();
  pip.finalizeInternalStatements();
  pip.DBConnection.close();
  do_check_false(pip.DBConnection.connectionReady);
}
