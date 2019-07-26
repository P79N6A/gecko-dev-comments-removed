



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

  
  add_ocsp_test("ocsp-stapling-revoked.example.com", getXPCOMStatusFromNSS(12), true);

  
  
  
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com", getXPCOMStatusFromNSS(18), true);

  
  
  add_test(function() {
    let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                   .getService(Ci.nsIX509CertDB);
    
    
    addCertFromFile(certdb, "tlsserver/other-test-ca.der", "CTu,u,u");
    run_next_test();
  });

  
  add_ocsp_test("ocsp-stapling-good-other-ca.example.com", getXPCOMStatusFromNSS(130), true);
  
  add_ocsp_test("ocsp-stapling-malformed.example.com", getXPCOMStatusFromNSS(120), true);
  
  add_ocsp_test("ocsp-stapling-srverr.example.com", getXPCOMStatusFromNSS(121), true);
  
  add_ocsp_test("ocsp-stapling-trylater.example.com", getXPCOMStatusFromNSS(122), true);
  
  add_ocsp_test("ocsp-stapling-needssig.example.com", getXPCOMStatusFromNSS(123), true);
  
  add_ocsp_test("ocsp-stapling-unauthorized.example.com", getXPCOMStatusFromNSS(124), true);
  
  add_ocsp_test("ocsp-stapling-unknown.example.com", getXPCOMStatusFromNSS(126), true);
  add_ocsp_test("ocsp-stapling-good-other.example.com", getXPCOMStatusFromNSS(126), true);
  
  add_ocsp_test("ocsp-stapling-none.example.com", Cr.NS_OK, true);
  
  add_ocsp_test("ocsp-stapling-empty.example.com", getXPCOMStatusFromNSS(129), true);
  
  add_ocsp_test("ocsp-stapling-expired.example.com", getXPCOMStatusFromNSS(132), true);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com", getXPCOMStatusFromNSS(132), true);

  run_next_test();
}
