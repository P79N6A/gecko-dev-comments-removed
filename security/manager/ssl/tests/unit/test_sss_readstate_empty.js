






let gSSService = null;

function checkStateRead(aSubject, aTopic, aData) {
  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "nonexistent.example.com", 0));
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "bugzilla.mozilla.org", 0));
  
  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "notexpired.example.com", 0));
  do_test_finished();
}

function run_test() {
  let profileDir = do_get_profile();
  let stateFile = profileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  
  
  do_check_false(stateFile.exists());
  stateFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0x1a4); 
  do_check_true(stateFile.exists());
  
  
  Services.obs.addObserver(checkStateRead, "data-storage-ready", false);
  do_test_pending();
  gSSService = Cc["@mozilla.org/ssservice;1"]
                 .getService(Ci.nsISiteSecurityService);
  do_check_true(gSSService != null);
}
