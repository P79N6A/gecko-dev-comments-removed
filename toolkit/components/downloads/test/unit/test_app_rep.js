





Cu.import('resource://gre/modules/NetUtil.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const gAppRep = Cc["@mozilla.org/downloads/application-reputation-service;1"].
                  getService(Ci.nsIApplicationReputationService);
let gHttpServ = null;
let gTables = {};

function readFileToString(aFilename) {
  let f = do_get_file(aFilename);
  let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                 .createInstance(Ci.nsIFileInputStream);
  stream.init(f, -1, 0, 0);
  let buf = NetUtil.readInputStreamToString(stream, stream.available());
  return buf;
}



function registerTableUpdate(aTable, aFilename) {
  
  if (!(aTable in gTables)) {
    gTables[aTable] = [];
  }

  
  let numChunks = gTables[aTable].length + 1;
  let redirectPath = "/" + aTable + "-" + numChunks;
  let redirectUrl = "localhost:4444" + redirectPath;

  
  
  gTables[aTable].push(redirectUrl);

  gHttpServ.registerPathHandler(redirectPath, function(request, response) {
    do_print("Mock safebrowsing server handling request for " + redirectPath);
    let contents = readFileToString(aFilename);
    do_print("Length of " + aFilename + ": " + contents.length);
    response.setHeader("Content-Type",
                       "application/vnd.google.safebrowsing-update", false);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(contents, contents.length);
  });
}

function run_test() {
  
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  
  
  Services.prefs.setBoolPref("browser.safebrowsing.malware.enabled", true);
  do_register_cleanup(function() {
    Services.prefs.clearUserPref("browser.safebrowsing.malware.enabled");
  });

  
  Services.prefs.setCharPref("urlclassifier.download_block_table",
                             "goog-badbinurl-shavar");
  do_register_cleanup(function() {
    Services.prefs.clearUserPref("urlclassifier.download_block_table");
  });

  gHttpServ = new HttpServer();
  gHttpServ.registerDirectory("/", do_get_cwd());
  gHttpServ.registerPathHandler("/download", function(request, response) {
    do_throw("This test should never make a remote lookup");
  });
  gHttpServ.start(4444);

  run_next_test();
}

add_test(function test_nullSourceURI() {
  gAppRep.queryReputation({
    
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_ERROR_UNEXPECTED, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_nullCallback() {
  try {
    gAppRep.queryReputation({
      sourceURI: createURI("http://example.com"),
      fileSize: 12,
    }, null);
    do_throw("Callback cannot be null");
  } catch (ex if ex.result == Cr.NS_ERROR_INVALID_POINTER) {
    run_next_test();
  }
});

add_test(function test_disabled() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL", "");
  gAppRep.queryReputation({
    sourceURI: createURI("http://example.com"),
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    
    do_check_eq(Cr.NS_ERROR_NOT_AVAILABLE, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});


add_test(function test_local_list() {
  
  function processUpdateRequest() {
    let response = "n:1000\n";
    for (let table in gTables) {
      response += "i:" + table + "\n";
      for (let i = 0; i < gTables[table].length; ++i) {
        response += "u:" + gTables[table][i] + "\n";
      }
    }
    do_print("Returning update response: " + response);
    return response;
  }
  gHttpServ.registerPathHandler("/downloads", function(request, response) {
    let buf = NetUtil.readInputStreamToString(request.bodyInputStream,
      request.bodyInputStream.available());
    let blob = processUpdateRequest();
    response.setHeader("Content-Type",
                       "application/vnd.google.safebrowsing-update", false);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(blob, blob.length);
  });

  let streamUpdater = Cc["@mozilla.org/url-classifier/streamupdater;1"]
    .getService(Ci.nsIUrlClassifierStreamUpdater);
  streamUpdater.updateUrl = "http://localhost:4444/downloads";

  
  
  registerTableUpdate("goog-badbinurl-shavar", "data/block_digest.chunk");
  
  registerTableUpdate("goog-downloadwhite-digest256", "data/digest.chunk");

  
  function updateSuccess(aEvent) {
    
    
    do_check_eq("1000", aEvent);
    do_print("All data processed");
    run_next_test();
  }
  
  function handleError(aEvent) {
    do_throw("We didn't download or update correctly: " + aEvent);
  }
  streamUpdater.downloadUpdates(
    "goog-downloadwhite-digest256,goog-badbinurl-shavar",
    "goog-downloadwhite-digest256,goog-badbinurl-shavar;\n",
    updateSuccess, handleError, handleError);
});

add_test(function test_unlisted() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  gAppRep.queryReputation({
    sourceURI: createURI("http://example.com"),
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_local_blacklist() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  gAppRep.queryReputation({
    sourceURI: createURI("http://blocklisted.com"),
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_true(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_referer_blacklist() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  gAppRep.queryReputation({
    sourceURI: createURI("http://example.com"),
    referrerURI: createURI("http://blocklisted.com"),
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_true(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_blocklist_trumps_allowlist() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  gAppRep.queryReputation({
    sourceURI: createURI("http://whitelisted.com"),
    referrerURI: createURI("http://blocklisted.com"),
    fileSize: 12,
  }, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_true(aShouldBlock);
    run_next_test();
  });
});
