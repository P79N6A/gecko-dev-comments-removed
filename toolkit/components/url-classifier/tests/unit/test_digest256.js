Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");

let gHttpServ = null;

let gDbService = Cc["@mozilla.org/url-classifier/dbservice;1"]
  .getService(Ci.nsIUrlClassifierDBService);

let gSecMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
  .getService(Ci.nsIScriptSecurityManager);


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
  let deferred = Promise.defer();
  
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
    response.setHeader("Content-Type",
                       "application/vnd.google.safebrowsing-update", false);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(contents, contents.length);
    deferred.resolve(contents);
  });
  return deferred.promise;
}


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


function run_test() {
  gHttpServ = new HttpServer();
  gHttpServ.registerDirectory("/", do_get_cwd());

  gHttpServ.registerPathHandler("/downloads", function(request, response) {
    let buf = NetUtil.readInputStreamToString(request.bodyInputStream,
      request.bodyInputStream.available());
    let blob = processUpdateRequest();
    response.setHeader("Content-Type",
                       "application/vnd.google.safebrowsing-update", false);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(blob, blob.length);
  });

  gHttpServ.start(4444);
  run_next_test();
}

function createURI(s) {
  let service = Cc["@mozilla.org/network/io-service;1"]
    .getService(Ci.nsIIOService);
  return service.newURI(s, null, null);
}


function handleError(aEvent) {
  do_throw("We didn't download or update correctly: " + aEvent);
}

add_test(function test_update() {
  let streamUpdater = Cc["@mozilla.org/url-classifier/streamupdater;1"]
    .getService(Ci.nsIUrlClassifierStreamUpdater);

  
  registerTableUpdate("goog-downloadwhite-digest256", "data/digest1.chunk");
  registerTableUpdate("goog-downloadwhite-digest256", "data/digest2.chunk");

  
  function updateSuccess(aEvent) {
    
    
    do_check_eq("1000", aEvent);
    do_print("All data processed");
    run_next_test();
  }
  streamUpdater.downloadUpdates(
    "goog-downloadwhite-digest256",
    "goog-downloadwhite-digest256;\n",
    "http://localhost:4444/downloads",
    updateSuccess, handleError, handleError);
});

add_test(function test_url_not_whitelisted() {
  let uri = createURI("http://example.com");
  let principal = gSecMan.getNoAppCodebasePrincipal(uri);
  gDbService.lookup(principal, "goog-downloadwhite-digest256",
    function handleEvent(aEvent) {
      
      do_check_eq("", aEvent);
      run_next_test();
    });
});

add_test(function test_url_whitelisted() {
  
  
  let uri = createURI("http://whitelisted.com");
  let principal = gSecMan.getNoAppCodebasePrincipal(uri);
  gDbService.lookup(principal, "goog-downloadwhite-digest256",
    function handleEvent(aEvent) {
      do_check_eq("goog-downloadwhite-digest256", aEvent);
      run_next_test();
    });
});
