




"use strict";





do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const SERVER_PORT = 8888;

function failingOCSPResponder() {
  return getFailingHttpServer(SERVER_PORT, ["www.example.com"]);
}

function start_ocsp_responder(expectedCertNames, expectedPaths) {
  return startOCSPResponder(SERVER_PORT, "www.example.com", [],
                            "test_ocsp_url", expectedCertNames, expectedPaths);
}

function check_cert_err(cert_name, expected_error) {
  let cert = constructCertFromFile("test_ocsp_url/" + cert_name + ".der");
  return checkCertErrorGeneric(certdb, cert, expected_error,
                               certificateUsageSSLServer);
}

function run_test() {
  addCertFromFile(certdb, "test_ocsp_url/ca.der", 'CTu,CTu,CTu');
  addCertFromFile(certdb, "test_ocsp_url/int.der", ',,');

  
  Services.prefs.setBoolPref("security.OCSP.require", true);

  Services.prefs.setCharPref("network.dns.localDomains",
                             "www.example.com");

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("bad-scheme",SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("empty-scheme-url", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("https-url", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(["hTTp-url"], ["hTTp-url"]);
    check_cert_err("hTTp-url", PRErrorCodeSuccess);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("negative-port", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    
    check_cert_err("no-host-url", SEC_ERROR_OCSP_SERVER_ERROR);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(["no-path-url"], ['']);
    check_cert_err("no-path-url", PRErrorCodeSuccess);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("no-scheme-host-port", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("no-scheme-url", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("unknown-scheme", SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    ocspResponder.stop(run_next_test);
  });

  run_next_test();
}
