



"use strict";





let gExpectOCSPRequest;

function add_ocsp_test(aHost, aExpectedResult, aStaplingEnabled) {
  add_connection_test(aHost, aExpectedResult,
    function() {
      gExpectOCSPRequest = !aStaplingEnabled;
      clearOCSPCache();
      clearSessionCache();
      Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling",
                                 aStaplingEnabled);
    });
}

function add_tests(certDB, otherTestCA) {
  
  add_ocsp_test("ocsp-stapling-good.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-revoked.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-malformed.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-srverr.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-trylater.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-needssig.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-unauthorized.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-unknown.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-good-other.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-none.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-expired.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-skip-responseBytes.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-critical-extension.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-noncritical-extension.example.com", Cr.NS_OK, false);
  add_ocsp_test("ocsp-stapling-empty-extensions.example.com", Cr.NS_OK, false);

  
  

  add_ocsp_test("ocsp-stapling-good.example.com", Cr.NS_OK, true);

  add_ocsp_test("ocsp-stapling-revoked.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE), true);

  
  
  
  

  
  
  add_test(function() {
    certDB.setCertTrust(otherTestCA, Ci.nsIX509Cert.CA_CERT,
                        Ci.nsIX509CertDB.UNTRUSTED);
    run_next_test();
  });
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);

  
  
  add_test(function() {
    certDB.setCertTrust(otherTestCA, Ci.nsIX509Cert.CA_CERT,
                        Ci.nsIX509CertDB.TRUSTED_SSL);
    run_next_test();
  });
  
  
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT),
                true);

  
  

  add_ocsp_test("ocsp-stapling-malformed.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_MALFORMED_REQUEST), true);
  add_ocsp_test("ocsp-stapling-srverr.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_SERVER_ERROR), true);
  add_ocsp_test("ocsp-stapling-trylater.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_TRY_SERVER_LATER), true);
  add_ocsp_test("ocsp-stapling-needssig.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_REQUEST_NEEDS_SIG), true);
  add_ocsp_test("ocsp-stapling-unauthorized.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST),
                true);
  add_ocsp_test("ocsp-stapling-unknown.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNKNOWN_CERT), true);
  add_ocsp_test("ocsp-stapling-good-other.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNKNOWN_CERT), true);
  
  
  
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK,
    function() {
      gExpectOCSPRequest = true;
      clearOCSPCache();
      clearSessionCache();
      Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);
    }
  );
  add_ocsp_test("ocsp-stapling-empty.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_MALFORMED_RESPONSE), true);

  add_ocsp_test("ocsp-stapling-skip-responseBytes.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_MALFORMED_RESPONSE), true);

  add_ocsp_test("ocsp-stapling-critical-extension.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION),
                true);
  add_ocsp_test("ocsp-stapling-noncritical-extension.example.com", Cr.NS_OK, true);
  
  add_ocsp_test("ocsp-stapling-empty-extensions.example.com", Cr.NS_OK, true);

  add_ocsp_test("ocsp-stapling-delegated-included.example.com", Cr.NS_OK, true);
  add_ocsp_test("ocsp-stapling-delegated-included-last.example.com", Cr.NS_OK, true);
  add_ocsp_test("ocsp-stapling-delegated-missing.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);
  add_ocsp_test("ocsp-stapling-delegated-missing-multiple.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);
  add_ocsp_test("ocsp-stapling-delegated-no-extKeyUsage.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);
  add_ocsp_test("ocsp-stapling-delegated-from-intermediate.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);
  add_ocsp_test("ocsp-stapling-delegated-keyUsage-crlSigning.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);
  add_ocsp_test("ocsp-stapling-delegated-wrong-extKeyUsage.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT), true);

  
  
  

  
  
  add_ocsp_test("keysize-ocsp-delegated.example.com",
                getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE),
                true);

  add_ocsp_test("revoked-ca-cert-used-as-end-entity.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE), true);
}

function check_ocsp_stapling_telemetry() {
  let histogram = Cc["@mozilla.org/base/telemetry;1"]
                    .getService(Ci.nsITelemetry)
                    .getHistogramById("SSL_OCSP_STAPLING")
                    .snapshot();
  do_check_eq(histogram.counts[0], 0); 
  do_check_eq(histogram.counts[1], 5); 
  do_check_eq(histogram.counts[2], 18); 
  do_check_eq(histogram.counts[3], 0); 
  do_check_eq(histogram.counts[4], 21); 
  run_next_test();
}

function run_test() {
  do_get_profile();

  let certDB = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);
  let otherTestCAFile = do_get_file("tlsserver/other-test-ca.der", false);
  let otherTestCADER = readFile(otherTestCAFile);
  let otherTestCA = certDB.constructX509(otherTestCADER, otherTestCADER.length);

  let fakeOCSPResponder = new HttpServer();
  fakeOCSPResponder.registerPrefixHandler("/", function (request, response) {
    response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
    do_check_true(gExpectOCSPRequest);
  });
  fakeOCSPResponder.start(8080);

  add_tls_server_setup("OCSPStaplingServer");

  add_tests(certDB, otherTestCA);

  add_test(function () {
    fakeOCSPResponder.stop(check_ocsp_stapling_telemetry);
  });

  run_next_test();
}
