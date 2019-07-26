


function run_test()
{
  
  let storage = LoginTest.initStorage(OUTDIR, "signons-empty.sqlite");
  do_check_true(storage instanceof Ci.nsIInterfaceRequestor);
  let db = storage.getInterface(Ci.mozIStorageConnection);
  do_check_neq(db, null);
  do_check_true(db.connectionReady);

  
  
  let lm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  do_check_true(lm instanceof Ci.nsIInterfaceRequestor);
  db = lm.getInterface(Ci.mozIStorageConnection);
  do_check_neq(db, null);
  do_check_true(db.connectionReady);
}
