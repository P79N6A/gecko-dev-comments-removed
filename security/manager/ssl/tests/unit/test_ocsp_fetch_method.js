




"use strict";





do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const SERVER_PORT = 8080;

function start_ocsp_responder(expectedCertNames, expectedPaths,
                              expectedMethods) {
  return startOCSPResponder(SERVER_PORT, "www.example.com", [],
                            "test_ocsp_fetch_method", expectedCertNames,
                            expectedPaths, expectedMethods);
}

function check_cert_err(cert_name, expected_error) {
  let cert = constructCertFromFile("test_ocsp_fetch_method/" + cert_name + ".der");
  return checkCertErrorGeneric(certdb, cert, expected_error,
                               certificateUsageSSLServer);
}

function run_test() {
  addCertFromFile(certdb, "test_ocsp_fetch_method/ca.der", 'CTu,CTu,CTu');
  addCertFromFile(certdb, "test_ocsp_fetch_method/int.der", ',,');

  
  Services.prefs.setBoolPref("security.OCSP.require", true);

  Services.prefs.setCharPref("network.dns.localDomains",
                             "www.example.com");

  add_tests_in_mode(true);
  add_tests_in_mode(false);
  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX)
{
  add_test(function() {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    run_next_test();
  });

  add_test(function() {
    clearOCSPCache();
    Services.prefs.setBoolPref("security.OCSP.GET.enabled", false);
    let ocspResponder = start_ocsp_responder(["a"], [], ["POST"]);
    check_cert_err("a", 0);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    Services.prefs.setBoolPref("security.OCSP.GET.enabled", true);
    let ocspResponder = start_ocsp_responder(["a"], [], ["GET"]);
    check_cert_err("a", 0);
    ocspResponder.stop(run_next_test);
  });

  
  add_test(function() {
    clearOCSPCache();
    Services.prefs.setBoolPref("security.OCSP.GET.enabled", true);
    
    if (!useMozillaPKIX) {
      let ocspResponder = start_ocsp_responder(["b", "a"], [], ["GET", "POST"]);
      check_cert_err("a", 0);
      ocspResponder.stop(run_next_test);
    } else {
      run_next_test();
    }
  });

}
