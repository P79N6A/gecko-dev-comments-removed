






const EXPECTED_ENTRIES = 5;
const EXPECTED_COLUMNS = 3;
let gProfileDir = null;





function checkStateWritten(aSubject, aTopic, aData) {
  do_check_eq(aData, SSS_STATE_FILE_NAME);

  let stateFile = gProfileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  do_check_true(stateFile.exists());
  let stateFileContents = readFile(stateFile);
  
  let lines = stateFileContents.split('\n').slice(0, -1);
  do_check_eq(lines.length, EXPECTED_ENTRIES);
  let sites = {}; 
  for (let line of lines) {
    let parts = line.split('\t');
    let host = parts[0];
    let score = parts[1];
    let lastAccessed = parts[2];
    let entry = parts[3].split(',');
    do_check_eq(entry.length, EXPECTED_COLUMNS);
    sites[host] = entry;
  }

  
  
  
  
  
  
  
  
  if (sites["bugzilla.mozilla.org"][1] != 1) {
    return;
  }
  if (sites["bugzilla.mozilla.org"][2] != 0) {
    return;
  }
  if (sites["a.example.com"][1] != 1) {
    return;
  }
  if (sites["a.example.com"][2] != 1) {
    return;
  }
  if (sites["b.example.com"][1] != 1) {
    return;
  }
  if (sites["b.example.com"][2] != 0) {
    return;
  }
  if (sites["c.c.example.com"][1] != 1) {
    return;
  }
  if (sites["c.c.example.com"][2] != 1) {
    return;
  }
  if (sites["d.example.com"][1] != 1) {
    return;
  }
  if (sites["d.example.com"][2] != 0) {
    return;
  }

  do_test_finished();
}

function run_test() {
  Services.prefs.setIntPref("test.datastorage.write_timer_ms", 100);
  gProfileDir = do_get_profile();
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);

  let uris = [ Services.io.newURI("http://bugzilla.mozilla.org", null, null),
               Services.io.newURI("http://a.example.com", null, null),
               Services.io.newURI("http://b.example.com", null, null),
               Services.io.newURI("http://c.c.example.com", null, null),
               Services.io.newURI("http://d.example.com", null, null) ];

  for (let i = 0; i < 1000; i++) {
    let uriIndex = i % uris.length;
    
    let maxAge = "max-age=" + (i * 1000);
     
    let includeSubdomains = (i % 2 == 0 ? "; includeSubdomains" : "");
    SSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS,
                            uris[uriIndex], maxAge + includeSubdomains, 0);
  }


  do_test_pending();
  Services.obs.addObserver(checkStateWritten, "data-storage-written", false);
}
