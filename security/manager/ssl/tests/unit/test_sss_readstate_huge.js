







function writeLine(aLine, aOutputStream) {
  aOutputStream.write(aLine, aLine.length);
}

let gSSService = null;

function checkStateRead(aSubject, aTopic, aData) {
  do_check_eq(aData, SSS_STATE_FILE_NAME);

  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example0.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example423.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example1023.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "example1024.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "example1025.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "example9000.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "example99999.example.com", 0));
  do_test_finished();
}

function run_test() {
  let profileDir = do_get_profile();
  let stateFile = profileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  
  
  do_check_false(stateFile.exists());
  let outputStream = FileUtils.openFileOutputStream(stateFile);
  let now = (new Date()).getTime();
  for (let i = 0; i < 10000; i++) {
    
    
    writeLine("example" + i + ".example.com\t0000000000000000000000000000000000000000000000000\t00000000000000000000000000000000000000\t" + (now + 100000) + ",1,0000000000000000000000000000000000000000000000000000000000000000000000000\n", outputStream);
  }
  outputStream.close();
  Services.obs.addObserver(checkStateRead, "data-storage-ready", false);
  do_test_pending();
  gSSService = Cc["@mozilla.org/ssservice;1"]
                 .getService(Ci.nsISiteSecurityService);
  do_check_true(gSSService != null);
}
