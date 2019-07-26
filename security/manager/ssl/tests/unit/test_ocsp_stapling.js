



"use strict";





function add_ocsp_test(aHost, aExpectedResult, aStaplingEnabled) {
  add_connection_test(aHost, aExpectedResult,
    function() {
      clearOCSPCache();
      Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling",
                                 aStaplingEnabled);
    });
}

function run_test() {
  do_get_profile();

  add_tls_server_setup("OCSPStaplingServer");

  
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

  
  

  add_ocsp_test("ocsp-stapling-good.example.com", Cr.NS_OK, true);

  add_ocsp_test("ocsp-stapling-revoked.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE), true);

  
  
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_BAD_DATABASE), true);

  
  
  add_test(function() {
    let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                   .getService(Ci.nsIX509CertDB);
    
    
    addCertFromFile(certdb, "tlsserver/other-test-ca.der", "CTu,u,u");
    run_next_test();
  });

  add_ocsp_test("ocsp-stapling-good-other-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE),
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
  
  add_ocsp_test("ocsp-stapling-none.example.com", Cr.NS_OK, true);
  add_ocsp_test("ocsp-stapling-empty.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_MALFORMED_RESPONSE), true);
  add_ocsp_test("ocsp-stapling-expired.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_OLD_RESPONSE), true);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_OCSP_OLD_RESPONSE), true);

  run_next_test();
}
