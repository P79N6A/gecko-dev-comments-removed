





Cu.import('resource://gre/modules/NetUtil.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const ApplicationReputationQuery = Components.Constructor(
      "@mozilla.org/downloads/application-reputation-query;1",
      "nsIApplicationReputationQuery");

const gAppRep = Cc["@mozilla.org/downloads/application-reputation-service;1"].
                  getService(Ci.nsIApplicationReputationService);
let gHttpServ = null;

function run_test() {
  
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL",
                             "http://localhost:4444/download");
  
  
  Services.prefs.setBoolPref("browser.safebrowsing.malware.enabled", true);
  do_register_cleanup(function() {
    Services.prefs.clearUserPref("browser.safebrowsing.malware.enabled");
  });

  gHttpServ = new HttpServer();
  gHttpServ.registerDirectory("/", do_get_cwd());

  function createVerdict(aShouldBlock) {
    
    
    blob = String.fromCharCode(parseInt(0x08, 16));
    if (aShouldBlock) {
      
      blob += String.fromCharCode(parseInt(0x01, 16));
    } else {
      
      blob += String.fromCharCode(parseInt(0x00, 16));
    }
    return blob;
  }

  gHttpServ.registerPathHandler("/download", function(request, response) {
    response.setHeader("Content-Type", "application/octet-stream", false);
    let buf = NetUtil.readInputStreamToString(
      request.bodyInputStream,
      request.bodyInputStream.available());
    do_print("Request length: " + buf.length);
    
    let blob = "this is not a serialized protocol buffer";
    
    
    if (buf.length == 35) {
      blob = createVerdict(true);
    } else if (buf.length == 38) {
      blob = createVerdict(false);
    }
    response.bodyOutputStream.write(blob, blob.length);
  });

  gHttpServ.start(4444);

  run_next_test();
}

add_test(function test_shouldBlock() {
  let query = new ApplicationReputationQuery();
  query.sourceURI = createURI("http://evil.com");
  query.fileSize = 12;

  gAppRep.queryReputation(query, function onComplete(aShouldBlock, aStatus) {
    do_check_true(aShouldBlock);
    do_check_eq(Cr.NS_OK, aStatus);
    run_next_test();
  });
});

add_test(function test_shouldNotBlock() {
  let query = new ApplicationReputationQuery();
  query.sourceURI = createURI("http://mozilla.com");
  query.fileSize = 12;

  gAppRep.queryReputation(query, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_OK, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_garbage() {
  let query = new ApplicationReputationQuery();
  query.sourceURI = createURI("http://thisisagarbageurl.com");
  query.fileSize = 12;

  gAppRep.queryReputation(query, function onComplete(aShouldBlock, aStatus) {
    
    do_check_eq(Cr.NS_ERROR_CANNOT_CONVERT_DATA, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_nullSourceURI() {
  let query = new ApplicationReputationQuery();
  query.fileSize = 12;
  
  gAppRep.queryReputation(query, function onComplete(aShouldBlock, aStatus) {
    do_check_eq(Cr.NS_ERROR_UNEXPECTED, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});

add_test(function test_nullCallback() {
  let query = new ApplicationReputationQuery();
  query.fileSize = 12;
  try {
    gAppRep.queryReputation(query, null);
    do_throw("Callback cannot be null");
  } catch (ex if ex.result == Cr.NS_ERROR_INVALID_POINTER) {
    run_next_test();
  }
});

add_test(function test_disabled() {
  Services.prefs.setCharPref("browser.safebrowsing.appRepURL", "");
  let query = new ApplicationReputationQuery();
  query.sourceURI = createURI("http://example.com");
  query.fileSize = 12;
  gAppRep.queryReputation(query, function onComplete(aShouldBlock, aStatus) {
    
    do_check_eq(Cr.NS_ERROR_NOT_AVAILABLE, aStatus);
    do_check_false(aShouldBlock);
    run_next_test();
  });
});
