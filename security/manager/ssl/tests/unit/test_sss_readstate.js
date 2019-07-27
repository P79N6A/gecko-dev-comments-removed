






function writeLine(aLine, aOutputStream) {
  aOutputStream.write(aLine, aLine.length);
}

let gSSService = null;

function checkStateRead(aSubject, aTopic, aData) {
  do_check_eq(aData, SSS_STATE_FILE_NAME);

  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "expired.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "notexpired.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "sub.bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "incsubdomain.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sub.incsubdomain.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "login.persona.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "sub.login.persona.org", 0));

  
  gSSService.clearAll();
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "expired.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "notexpired.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sub.bugzilla.mozilla.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "incsubdomain.example.com", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "sub.incsubdomain.example.com", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "login.persona.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sub.login.persona.org", 0));
  do_test_finished();
}

function run_test() {
  let profileDir = do_get_profile();
  let stateFile = profileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  
  
  do_check_false(stateFile.exists());
  let outputStream = FileUtils.openFileOutputStream(stateFile);
  let now = (new Date()).getTime();
  writeLine("expired.example.com:HSTS\t0\t0\t" + (now - 100000) + ",1,0\n", outputStream);
  writeLine("notexpired.example.com:HSTS\t0\t0\t" + (now + 100000) + ",1,0\n", outputStream);
  
  writeLine("bugzilla.mozilla.org:HSTS\t0\t0\t" + (now + 100000) + ",1,0\n", outputStream);
  writeLine("incsubdomain.example.com:HSTS\t0\t0\t" + (now + 100000) + ",1,1\n", outputStream);
  
  writeLine("login.persona.org:HSTS\t0\t0\t0,2,0\n", outputStream);
  outputStream.close();
  Services.obs.addObserver(checkStateRead, "data-storage-ready", false);
  do_test_pending();
  gSSService = Cc["@mozilla.org/ssservice;1"]
                 .getService(Ci.nsISiteSecurityService);
  do_check_true(gSSService != null);
}
