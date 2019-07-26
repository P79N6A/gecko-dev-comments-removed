"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const isB2G = ("@mozilla.org/b2g-keyboard;1" in Components.classes);

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

function run_test() {
  run_next_test();
}











const NS_ERROR_SEC_ERROR_UNKNOWN_ISSUER = 0x80000000 
				        + (    (0x45 + 21) << 16)
				        + (-(-0x2000 + 13)      );

function check_open_result(name, expectedRv) {
  if (expectedRv == Cr.NS_OK && !isB2G) {
    
    expectedRv = NS_ERROR_SEC_ERROR_UNKNOWN_ISSUER;
  }

  return function openSignedJARFileCallback(rv, aZipReader, aSignerCert) {
    do_print("openSignedJARFileCallback called for " + name);
    do_check_eq(rv, expectedRv);
    do_check_eq(aZipReader != null,  Components.isSuccessCode(expectedRv));
    do_check_eq(aSignerCert != null, Components.isSuccessCode(expectedRv));
    run_next_test();
  };
}

function original_app_path(test_name) {
  return do_get_file("test_signed_apps/" + test_name + ".zip", false);
}



add_test(function () {
  certdb.openSignedJARFileAsync(
    original_app_path("test-privileged-app-test-1.0"),
    check_open_result("test-privileged-app-test-1.0",
                      NS_ERROR_SEC_ERROR_UNKNOWN_ISSUER));
});


add_test(function () {
  certdb.openSignedJARFileAsync(
    original_app_path("privileged-app-test-1.0"),
    check_open_result("privileged-app-test-1.0", Cr.NS_OK));
});
