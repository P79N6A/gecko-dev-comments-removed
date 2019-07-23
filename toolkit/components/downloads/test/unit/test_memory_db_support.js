






































const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function run_test() {
  let observer = dm.QueryInterface(Ci.nsIObserver);

  
  let connDisk = dm.DBConnection;
  do_check_true(connDisk.connectionReady);
  do_check_neq(connDisk.databaseFile, null);

  
  observer.observe(null, "dlmgr-switchdb", "disk");

  
  do_check_true(dm.DBConnection.connectionReady);
  do_check_neq(dm.DBConnection.databaseFile, null);
  do_check_true(connDisk.databaseFile.equals(dm.DBConnection.databaseFile));
  connDisk = dm.DBConnection;

  
  observer.observe(null, "dlmgr-switchdb", "memory");

  
  let connMemory = dm.DBConnection;
  do_check_true(connMemory.connectionReady);
  do_check_eq(connMemory.databaseFile, null);

  
  observer.observe(null, "dlmgr-switchdb", "memory");

  
  connMemory = dm.DBConnection;
  do_check_true(connMemory.connectionReady);
  do_check_eq(connMemory.databaseFile, null);

  
  observer.observe(null, "dlmgr-switchdb", "disk");

  
  do_check_true(dm.DBConnection.connectionReady);
  do_check_neq(dm.DBConnection.databaseFile, null);
  do_check_true(connDisk.databaseFile.equals(dm.DBConnection.databaseFile));
}
