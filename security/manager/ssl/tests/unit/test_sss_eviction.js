






let gSSService = null;
let gProfileDir = null;

function do_state_written(aSubject, aTopic, aData) {
  do_check_eq(aData, SSS_STATE_FILE_NAME);

  let stateFile = gProfileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  do_check_true(stateFile.exists());
  let stateFileContents = readFile(stateFile);
  
  let lines = stateFileContents.split('\n').slice(0, -1);
  
  
  
  
  
  
  if (lines.length != 1024) {
    return;
  }

  let foundLegitSite = false;
  for (let line of lines) {
    if (line.startsWith("frequentlyused.example.com")) {
      foundLegitSite = true;
      break;
    }
  }

  do_check_true(foundLegitSite);
  do_test_finished();
}

function do_state_read(aSubject, aTopic, aData) {
  do_check_eq(aData, SSS_STATE_FILE_NAME);

  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "frequentlyused.example.com", 0));
  for (let i = 0; i < 2000; i++) {
    let uri = Services.io.newURI("http://bad" + i + ".example.com", null, null);
    gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                            "max-age=1000", 0);
  }
  do_test_pending();
  Services.obs.addObserver(do_state_written, "data-storage-written", false);
  do_test_finished();
}

function run_test() {
  Services.prefs.setIntPref("test.datastorage.write_timer_ms", 100);
  gProfileDir = do_get_profile();
  let stateFile = gProfileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  
  
  do_check_false(stateFile.exists());
  let outputStream = FileUtils.openFileOutputStream(stateFile);
  let now = (new Date()).getTime();
  let line = "frequentlyused.example.com\t4\t0\t" + (now + 100000) + ",1,0\n";
  outputStream.write(line, line.length);
  outputStream.close();
  Services.obs.addObserver(do_state_read, "data-storage-ready", false);
  do_test_pending();
  gSSService = Cc["@mozilla.org/ssservice;1"]
                 .getService(Ci.nsISiteSecurityService);
  do_check_true(gSSService != null);
}
