












let { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});


let id = "xpcshell@tests.mozilla.org";
let appName = "XPCShell";
let version = "1";
let platformVersion = "1.9.2";
let appInfo = {
  
  vendor: "Mozilla",
  name: appName,
  ID: id,
  version: version,
  appBuildID: "2007010101",
  platformVersion: platformVersion ? platformVersion : "1.0",
  platformBuildID: "2007010101",

  
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",
  invalidateCachesOnRestart: function invalidateCachesOnRestart() {
    
  },

  
  annotations: {},

  annotateCrashReport: function(key, data) {
    this.annotations[key] = data;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULAppInfo,
                                         Ci.nsIXULRuntime,
                                         Ci.nsICrashReporter,
                                         Ci.nsISupports])
};

let XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    appInfo.QueryInterface(iid);
    if (outer != null) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return appInfo.QueryInterface(iid);
  }
};

let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                          XULAPPINFO_CONTRACTID, XULAppInfoFactory);



let profile = do_get_profile();
let revocations = profile.clone();
revocations.append("revocations.txt");
if (!revocations.exists()) {
  let existing = do_get_file("test_onecrl/sample_revocations.txt", false);
  existing.copyTo(profile,"revocations.txt");
}

let certDB = Cc["@mozilla.org/security/x509certdb;1"]
               .getService(Ci.nsIX509CertDB);


let testserver = new HttpServer();

let blocklist_contents =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
    "<blocklist xmlns=\"http://www.mozilla.org/2006/addons-blocklist\">" +
    
    "<certItems><certItem issuerName='Some nonsense in issuer'>" +
    "<serialNumber>AkHVNA==</serialNumber>" +
    "</certItem><certItem issuerName='MA0xCzAJBgNVBAMMAmNh'>" +
    "<serialNumber>some nonsense in serial</serialNumber>" +
    "</certItem><certItem issuerName='some nonsense in both issuer'>" +
    "<serialNumber>and serial</serialNumber></certItem>" +
    
    
    
    "<certItem issuerName='MBIxEDAOBgNVBAMTB1Rlc3QgQ0E='>" +
    "<serialNumber>oops! more nonsense.</serialNumber>" +
    "<serialNumber>X1o=</serialNumber></certItem>" +
    
    
    
    "<certItem issuerName='MBgxFjAUBgNVBAMTDU90aGVyIHRlc3QgQ0E='>" +
    "<serialNumber>AKEIivg=</serialNumber></certItem>" +
    
    
    
    
    
    
    
    "<certItem issuerName='YW5vdGhlciBpbWFnaW5hcnkgaXNzdWVy'>" +
    "<serialNumber>c2VyaWFsMi4=</serialNumber>" +
    "<serialNumber>YW5vdGhlciBzZXJpYWwu</serialNumber>" +
    "</certItem></certItems></blocklist>";
testserver.registerPathHandler("/push_blocked_cert/",
  function serveResponse(request, response) {
    response.write(blocklist_contents);
  });


testserver.start(-1);
let port = testserver.identity.primaryPort;


let addonManager = Cc["@mozilla.org/addons/integration;1"]
                     .getService(Ci.nsIObserver)
                     .QueryInterface(Ci.nsITimerCallback);
addonManager.observe(null, "addons-startup", null);

let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                  .createInstance(Ci.nsIScriptableUnicodeConverter);
converter.charset = "UTF-8";

function verify_cert(file, expectedError) {
  let cert_der = readFile(do_get_file(file));
  let ee = certDB.constructX509(cert_der, cert_der.length);
  equal(expectedError, certDB.verifyCertNow(ee, certificateUsageSSLServer,
                                            NO_FLAGS, {}, {}));
}

function load_cert(cert, trust) {
  let file = "tlsserver/" + cert + ".der";
  addCertFromFile(certDB, file, trust);
}

function test_is_revoked(certList, issuerString, serialString) {
  let issuer = converter.convertToByteArray(issuerString, {});
  let serial = converter.convertToByteArray(serialString, {});
  return certList.isCertRevoked(issuer,
                                issuerString.length,
                                serial,
                                serialString.length);
}

function run_test() {
  
  load_cert("test-ca", "CTu,CTu,CTu");
  load_cert("test-int", ",,");
  load_cert("other-test-ca", "CTu,CTu,CTu");

  let certList = Cc["@mozilla.org/security/certblocklist;1"]
                   .getService(Ci.nsICertBlocklist);

  
  
  
  
  
  
  ok(test_is_revoked(certList, "some imaginary issuer", "serial."),
     "issuer / serial pair should be blocked");

  
  
  
  ok(test_is_revoked(certList, "another imaginary issuer", "serial."),
     "issuer / serial pair should be blocked");

  
  
  
  
  ok(test_is_revoked(certList, "another imaginary issuer", "serial2."),
     "issuer / serial pair should be blocked");

  
  
  
  let file = "tlsserver/test-int-ee.der";
  verify_cert(file, Cr.NS_OK);

  
  
  file = "tlsserver/default-ee.der";
  verify_cert(file, Cr.NS_OK);

  
  add_test(function() {
    let certblockObserver = {
      observe: function(aSubject, aTopic, aData) {
        Services.obs.removeObserver(this, "blocklist-updated");
        run_next_test();
      }
    }

    Services.obs.addObserver(certblockObserver, "blocklist-updated", false);
    Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:" +
                               port + "/push_blocked_cert/");
    let blocklist = Cc["@mozilla.org/extensions/blocklist;1"]
                      .getService(Ci.nsITimerCallback);
    blocklist.notify(null);
  });

  add_test(function() {
    
    
    
    ok(test_is_revoked(certList, "another imaginary issuer", "serial2."),
      "issuer / serial pair should be blocked");

    
    
    ok(test_is_revoked(certList, "another imaginary issuer", "serial2."),
       "issuer / serial pair should be blocked");
    ok(test_is_revoked(certList, "another imaginary issuer", "another serial."),
       "issuer / serial pair should be blocked");

    
    
    let profile = do_get_profile();
    let revocations = profile.clone();
    revocations.append("revocations.txt");
    ok(revocations.exists(), "the revocations file should exist");
    let inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                        .createInstance(Ci.nsIFileInputStream);
    inputStream.init(revocations,-1, -1, 0);
    inputStream.QueryInterface(Ci.nsILineInputStream);
    let contents = "";
    let hasmore = false;
    do {
      var line = {};
      hasmore = inputStream.readLine(line);
      contents = contents + (contents.length == 0 ? "" : "\n") + line.value;
    } while (hasmore);
    let expected = "# Auto generated contents. Do not edit.\n" +
                  "MBgxFjAUBgNVBAMTDU90aGVyIHRlc3QgQ0E=\n" +
                  " AKEIivg=\n" +
                  "MBIxEDAOBgNVBAMTB1Rlc3QgQ0E=\n" +
                  " X1o=\n" +
                  "YW5vdGhlciBpbWFnaW5hcnkgaXNzdWVy\n" +
                  " YW5vdGhlciBzZXJpYWwu\n" +
                  " c2VyaWFsMi4=";
    equal(contents, expected, "revocations.txt should be as expected");

    
    let file = "tlsserver/test-int-ee.der";
    verify_cert(file, SEC_ERROR_REVOKED_CERTIFICATE);

    
    file = "tlsserver/other-issuer-ee.der";
    verify_cert(file, SEC_ERROR_REVOKED_CERTIFICATE);

    
    file = "tlsserver/default-ee.der";
    verify_cert(file, Cr.NS_OK);

    
    file = "tlsserver/unknown-issuer.der";
    verify_cert(file, SEC_ERROR_UNKNOWN_ISSUER);

    
    let lastModified = revocations.lastModifiedTime;
    
    certList.addRevokedCert("YW5vdGhlciBpbWFnaW5hcnkgaXNzdWVy","c2VyaWFsMi4=");
    certList.saveEntries();
    let newModified = revocations.lastModifiedTime;
    equal(lastModified, newModified,
          "saveEntries with no modifications should not update the backing file");

    run_next_test();
  });

  
  run_next_test();
}
