"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

function run_test() {
  run_next_test();
}

function check_open_result(name, expectedRv) {
  return function openSignedAppFileCallback(rv, aZipReader, aSignerCert) {
    do_print("openSignedAppFileCallback called for " + name);
    equal(rv, expectedRv, "Actual and expected return value should match");
    equal(aZipReader != null, Components.isSuccessCode(expectedRv),
          "ZIP reader should be null only if the return value denotes failure");
    equal(aSignerCert != null, Components.isSuccessCode(expectedRv),
          "Signer cert should be null only if the return value denotes failure");
    run_next_test();
  };
}

function original_app_path(test_name) {
  return do_get_file("test_signed_apps/" + test_name + ".zip", false);
}



add_test(function () {
  certdb.openSignedAppFileAsync(
    Ci.nsIX509CertDB.AppMarketplaceProdPublicRoot,
    original_app_path("test-privileged-app-test-1.0"),
    check_open_result("test-privileged-app-test-1.0",
                      getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER)));
});


add_test(function () {
  certdb.openSignedAppFileAsync(
    Ci.nsIX509CertDB.AppMarketplaceProdPublicRoot,
    original_app_path("privileged-app-test-1.0"),
    check_open_result("privileged-app-test-1.0", Cr.NS_OK));
});
