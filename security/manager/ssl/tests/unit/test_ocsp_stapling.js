



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
  
  add_ocsp_test("ocsp-stapling-good.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-revoked.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-malformed.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-srverr.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-trylater.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-needssig.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-unauthorized.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-unknown.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-good-other.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-none.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-expired.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-skip-responseBytes.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-critical-extension.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-noncritical-extension.example.com",
                PRErrorCodeSuccess, false);
  add_ocsp_test("ocsp-stapling-empty-extensions.example.com",
                PRErrorCodeSuccess, false);

  
  

  add_ocsp_test("ocsp-stapling-good.example.com", PRErrorCodeSuccess, true);

  add_ocsp_test("ocsp-stapling-revoked.example.com",
                SEC_ERROR_REVOKED_CERTIFICATE, true);

  
  
  
  

  
  
  add_test(function() {
    certDB.setCertTrust(otherTestCA, Ci.nsIX509Cert.CA_CERT,
                        Ci.nsIX509CertDB.UNTRUSTED);
    run_next_test();
  });
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);

  
  
  add_test(function() {
    certDB.setCertTrust(otherTestCA, Ci.nsIX509Cert.CA_CERT,
                        Ci.nsIX509CertDB.TRUSTED_SSL);
    run_next_test();
  });
  
  
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);

  
  

  add_ocsp_test("ocsp-stapling-malformed.example.com",
                SEC_ERROR_OCSP_MALFORMED_REQUEST, true);
  add_ocsp_test("ocsp-stapling-srverr.example.com",
                SEC_ERROR_OCSP_SERVER_ERROR, true);
  add_ocsp_test("ocsp-stapling-trylater.example.com",
                SEC_ERROR_OCSP_TRY_SERVER_LATER, true);
  add_ocsp_test("ocsp-stapling-needssig.example.com",
                SEC_ERROR_OCSP_REQUEST_NEEDS_SIG, true);
  add_ocsp_test("ocsp-stapling-unauthorized.example.com",
                SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST, true);
  add_ocsp_test("ocsp-stapling-unknown.example.com",
                SEC_ERROR_OCSP_UNKNOWN_CERT, true);
  add_ocsp_test("ocsp-stapling-good-other.example.com",
                MOZILLA_PKIX_ERROR_OCSP_RESPONSE_FOR_CERT_MISSING, true);
  
  
  
  add_connection_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess,
    function() {
      gExpectOCSPRequest = true;
      clearOCSPCache();
      clearSessionCache();
      Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);
    }
  );
  add_ocsp_test("ocsp-stapling-empty.example.com",
                SEC_ERROR_OCSP_MALFORMED_RESPONSE, true);

  add_ocsp_test("ocsp-stapling-skip-responseBytes.example.com",
                SEC_ERROR_OCSP_MALFORMED_RESPONSE, true);

  add_ocsp_test("ocsp-stapling-critical-extension.example.com",
                SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION, true);
  add_ocsp_test("ocsp-stapling-noncritical-extension.example.com",
                PRErrorCodeSuccess, true);
  
  add_ocsp_test("ocsp-stapling-empty-extensions.example.com",
                PRErrorCodeSuccess, true);

  add_ocsp_test("ocsp-stapling-delegated-included.example.com",
                PRErrorCodeSuccess, true);
  add_ocsp_test("ocsp-stapling-delegated-included-last.example.com",
                PRErrorCodeSuccess, true);
  add_ocsp_test("ocsp-stapling-delegated-missing.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);
  add_ocsp_test("ocsp-stapling-delegated-missing-multiple.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);
  add_ocsp_test("ocsp-stapling-delegated-no-extKeyUsage.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);
  add_ocsp_test("ocsp-stapling-delegated-from-intermediate.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);
  add_ocsp_test("ocsp-stapling-delegated-keyUsage-crlSigning.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);
  add_ocsp_test("ocsp-stapling-delegated-wrong-extKeyUsage.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);

  
  
  

  
  
  add_ocsp_test("keysize-ocsp-delegated.example.com",
                SEC_ERROR_OCSP_INVALID_SIGNING_CERT, true);

  add_ocsp_test("revoked-ca-cert-used-as-end-entity.example.com",
                SEC_ERROR_REVOKED_CERTIFICATE, true);
}

function check_ocsp_stapling_telemetry() {
  let histogram = Cc["@mozilla.org/base/telemetry;1"]
                    .getService(Ci.nsITelemetry)
                    .getHistogramById("SSL_OCSP_STAPLING")
                    .snapshot();
  equal(histogram.counts[0], 0,
        "Should have 0 connections for unused histogram bucket 0");
  equal(histogram.counts[1], 5,
        "Actual and expected connections with a good response should match");
  equal(histogram.counts[2], 18,
        "Actual and expected connections with no stapled response should match");
  equal(histogram.counts[3], 0,
        "Actual and expected connections with an expired response should match");
  equal(histogram.counts[4], 21,
        "Actual and expected connections with bad responses should match");
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
    ok(gExpectOCSPRequest,
       "Should be getting an OCSP request only when expected");
  });
  fakeOCSPResponder.start(8888);

  add_tls_server_setup("OCSPStaplingServer");

  add_tests(certDB, otherTestCA);

  add_test(function () {
    fakeOCSPResponder.stop(check_ocsp_stapling_telemetry);
  });

  run_next_test();
}
