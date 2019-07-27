












Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const BackgroundFileSaverOutputStream = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=outputstream",
      "nsIBackgroundFileSaver");

const StringInputStream = Components.Constructor(
      "@mozilla.org/io/string-input-stream;1",
      "nsIStringInputStream",
      "setData");

const TEST_FILE_NAME_1 = "test-backgroundfilesaver-1.txt";

const gAppRep = Cc["@mozilla.org/downloads/application-reputation-service;1"].
                  getService(Ci.nsIApplicationReputationService);
let gStillRunning = true;
let gTables = {};
let gHttpServer = null;





function getTempFile(aLeafName) {
  let file = FileUtils.getFile("TmpD", [aLeafName]);
  do_register_cleanup(function GTF_cleanup() {
    if (file.exists()) {
      file.remove(false);
    }
  });
  return file;
}

function readFileToString(aFilename) {
  let f = do_get_file(aFilename);
  let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                 .createInstance(Ci.nsIFileInputStream);
  stream.init(f, -1, 0, 0);
  let buf = NetUtil.readInputStreamToString(stream, stream.available());
  return buf;
}













function promiseSaverComplete(aSaver, aOnTargetChangeFn) {
  let deferred = Promise.defer();
  aSaver.observer = {
    onTargetChange: function BFSO_onSaveComplete(aSaver, aTarget)
    {
      if (aOnTargetChangeFn) {
        aOnTargetChangeFn(aTarget);
      }
    },
    onSaveComplete: function BFSO_onSaveComplete(aSaver, aStatus)
    {
      if (Components.isSuccessCode(aStatus)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception("Saver failed.", aStatus));
      }
    },
  };
  return deferred.promise;
}















function promiseCopyToSaver(aSourceString, aSaverOutputStream, aCloseWhenDone) {
  let deferred = Promise.defer();
  let inputStream = new StringInputStream(aSourceString, aSourceString.length);
  let copier = Cc["@mozilla.org/network/async-stream-copier;1"]
               .createInstance(Ci.nsIAsyncStreamCopier);
  copier.init(inputStream, aSaverOutputStream, null, false, true, 0x8000, true,
              aCloseWhenDone);
  copier.asyncCopy({
    onStartRequest: function () { },
    onStopRequest: function (aRequest, aContext, aStatusCode)
    {
      if (Components.isSuccessCode(aStatusCode)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception(aResult));
      }
    },
  }, null);
  return deferred.promise;
}


function registerTableUpdate(aTable, aFilename) {
  
  if (!(aTable in gTables)) {
    gTables[aTable] = [];
  }

  
  let numChunks = gTables[aTable].length + 1;
  let redirectPath = "/" + aTable + "-" + numChunks;
  let redirectUrl = "localhost:4444" + redirectPath;

  
  
  gTables[aTable].push(redirectUrl);

  gHttpServer.registerPathHandler(redirectPath, function(request, response) {
    do_print("Mock safebrowsing server handling request for " + redirectPath);
    let contents = readFileToString(aFilename);
    do_print("Length of " + aFilename + ": " + contents.length);
    response.setHeader("Content-Type",
                       "application/vnd.google.safebrowsing-update", false);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(contents, contents.length);
  });
}




function run_test()
{
  run_next_test();
}

add_task(function test_setup()
{
  
  do_timeout(10 * 60 * 1000, function() {
    if (gStillRunning) {
      do_throw("Test timed out.");
    }
  });
  
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  
  
  Services.prefs.setBoolPref("browser.safebrowsing.malware.enabled", true);
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.enabled", true);
  
  
  Services.prefs.setCharPref("urlclassifier.downloadBlockTable",
                             "goog-badbinurl-shavar");
  Services.prefs.setCharPref("urlclassifier.downloadAllowTable",
                             "goog-downloadwhite-digest256");
  
  let locale = Services.prefs.getCharPref("general.useragent.locale");
  Services.prefs.setCharPref("general.useragent.locale", "en-US");

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("browser.safebrowsing.malware.enabled");
    Services.prefs.clearUserPref("browser.safebrowsing.downloads.enabled");
    Services.prefs.clearUserPref("urlclassifier.downloadBlockTable");
    Services.prefs.clearUserPref("urlclassifier.downloadAllowTable");
    Services.prefs.setCharPref("general.useragent.locale", locale);
  });

  gHttpServer = new HttpServer();
  gHttpServer.registerDirectory("/", do_get_cwd());

  function createVerdict(aShouldBlock) {
    
    
    let blob = String.fromCharCode(parseInt(0x08, 16));
    if (aShouldBlock) {
      
      blob += String.fromCharCode(parseInt(0x01, 16));
    } else {
      
      blob += String.fromCharCode(parseInt(0x00, 16));
    }
    return blob;
  }

  gHttpServer.registerPathHandler("/throw", function(request, response) {
    do_throw("We shouldn't be getting here");
  });

  gHttpServer.registerPathHandler("/download", function(request, response) {
    do_print("Querying remote server for verdict");
    response.setHeader("Content-Type", "application/octet-stream", false);
    let buf = NetUtil.readInputStreamToString(
      request.bodyInputStream,
      request.bodyInputStream.available());
    do_print("Request length: " + buf.length);
    
    
    let blob = "this is not a serialized protocol buffer (the length doesn't match our hard-coded values)";
    
    
    if (buf.length == 67) {
      
      blob = createVerdict(true);
    } else if (buf.length == 73) {
      
      blob = createVerdict(false);
    }
    response.bodyOutputStream.write(blob, blob.length);
  });

  gHttpServer.start(4444);
});


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


function waitForUpdates() {
  let deferred = Promise.defer();
  gHttpServer.registerPathHandler("/downloads", function(request, response) {
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

  
  
  
  registerTableUpdate("goog-downloadwhite-digest256", "data/digest.chunk");

  
  function updateSuccess(aEvent) {
    
    
    do_check_eq("1000", aEvent);
    do_print("All data processed");
    deferred.resolve(true);
  }
  
  function handleError(aEvent) {
    do_throw("We didn't download or update correctly: " + aEvent);
    deferred.reject();
  }
  streamUpdater.downloadUpdates(
    "goog-downloadwhite-digest256",
    "goog-downloadwhite-digest256;\n",
    "http://localhost:4444/downloads",
    updateSuccess, handleError, handleError);
  return deferred.promise;
}

function promiseQueryReputation(query, expectedShouldBlock) {
  let deferred = Promise.defer();
  function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_eq(aShouldBlock, expectedShouldBlock);
    deferred.resolve(true);
  }
  gAppRep.queryReputation(query, onComplete);
  return deferred.promise;
}

add_task(function()
{
  
  yield waitForUpdates();
});

add_task(function test_signature_whitelists()
{
  
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             true);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/throw");

  
  let destFile = getTempFile(TEST_FILE_NAME_1);

  let data = readFileToString("data/signed_win.exe");
  let saver = new BackgroundFileSaverOutputStream();
  let completionPromise = promiseSaverComplete(saver);
  saver.enableSignatureInfo();
  saver.setTarget(destFile, false);
  yield promiseCopyToSaver(data, saver, true);

  saver.finish(Cr.NS_OK);
  yield completionPromise;

  
  destFile.remove(false);

  
  
  yield promiseQueryReputation({sourceURI: createURI("http://evil.com"),
                                signatureInfo: saver.signatureInfo,
                                fileSize: 12}, false);
});

add_task(function test_blocked_binary()
{
  
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             true);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  
  yield promiseQueryReputation({sourceURI: createURI("http://evil.com"),
                                suggestedFileName: "noop.bat",
                                fileSize: 12}, true);
});

add_task(function test_non_binary()
{
  
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             true);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/throw");
  yield promiseQueryReputation({sourceURI: createURI("http://evil.com"),
                                suggestedFileName: "noop.txt",
                                fileSize: 12}, false);
});

add_task(function test_good_binary()
{
  
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             true);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  
  yield promiseQueryReputation({sourceURI: createURI("http://mozilla.com"),
                                suggestedFileName: "noop.bat",
                                fileSize: 12}, false);
});

add_task(function test_disabled()
{
  
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             false);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/throw");
  let query = {sourceURI: createURI("http://example.com"),
               suggestedFileName: "noop.bat",
               fileSize: 12};
  let deferred = Promise.defer();
  gAppRep.queryReputation(query,
    function onComplete(aShouldBlock, aStatus) {
      
      do_check_eq(Cr.NS_ERROR_NOT_AVAILABLE, aStatus);
      do_check_false(aShouldBlock);
      deferred.resolve(true);
    }
  );
  yield deferred.promise;
});

add_task(function test_disabled_through_lists()
{
  Services.prefs.setBoolPref("browser.safebrowsing.downloads.remote.enabled",
                             false);
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  Services.prefs.setCharPref("urlclassifier.downloadBlockTable", "");
  let query = {sourceURI: createURI("http://example.com"),
               suggestedFileName: "noop.bat",
               fileSize: 12};
  let deferred = Promise.defer();
  gAppRep.queryReputation(query,
    function onComplete(aShouldBlock, aStatus) {
      
      do_check_eq(Cr.NS_ERROR_NOT_AVAILABLE, aStatus);
      do_check_false(aShouldBlock);
      deferred.resolve(true);
    }
  );
  yield deferred.promise;
});
add_task(function test_teardown()
{
  gStillRunning = false;
});
