



"use strict";








do_get_profile();
let certOverrideService = Cc["@mozilla.org/security/certoverride;1"]
                            .getService(Ci.nsICertOverrideService);

function add_cert_override(aHost, aExpectedBits, aSecurityInfo) {
  let sslstatus = aSecurityInfo.QueryInterface(Ci.nsISSLStatusProvider)
                               .SSLStatus;
  let bits =
    (sslstatus.isUntrusted ? Ci.nsICertOverrideService.ERROR_UNTRUSTED : 0) |
    (sslstatus.isDomainMismatch ? Ci.nsICertOverrideService.ERROR_MISMATCH : 0) |
    (sslstatus.isNotValidAtThisTime ? Ci.nsICertOverrideService.ERROR_TIME : 0);
  do_check_eq(bits, aExpectedBits);
  let cert = sslstatus.serverCert;
  certOverrideService.rememberValidityOverride(aHost, 8443, cert, aExpectedBits,
                                               true);
}

function add_cert_override_test(aHost, aExpectedBits, aExpectedError) {
  add_connection_test(aHost, aExpectedError, null,
                      add_cert_override.bind(this, aHost, aExpectedBits));
  add_connection_test(aHost, Cr.NS_OK);
}

function add_non_overridable_test(aHost, aExpectedError) {
  add_connection_test(
    aHost, getXPCOMStatusFromNSS(aExpectedError), null,
    function (securityInfo) {
      
      
      
      securityInfo.QueryInterface(Ci.nsISSLStatusProvider);
      do_check_eq(securityInfo.SSLStatus, null);
    });
}

function check_telemetry() {
  let histogram = Cc["@mozilla.org/base/telemetry;1"]
                    .getService(Ci.nsITelemetry)
                    .getHistogramById("SSL_CERT_ERROR_OVERRIDES")
                    .snapshot();
  do_check_eq(histogram.counts[ 0], 0);
  do_check_eq(histogram.counts[ 2], 8 + 1); 
  do_check_eq(histogram.counts[ 3], 0);     
  do_check_eq(histogram.counts[ 4], 0 + 5); 
  do_check_eq(histogram.counts[ 5], 0 + 1); 
  do_check_eq(histogram.counts[ 6], 0 + 1); 
  do_check_eq(histogram.counts[ 7], 0 + 1); 
  do_check_eq(histogram.counts[ 8], 2 + 2); 
  do_check_eq(histogram.counts[ 9], 4 + 4); 
  do_check_eq(histogram.counts[10], 5 + 5); 
  run_next_test();
}

function run_test() {
  add_tls_server_setup("BadCertServer");

  let fakeOCSPResponder = new HttpServer();
  fakeOCSPResponder.registerPrefixHandler("/", function (request, response) {
    response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
  });
  fakeOCSPResponder.start(8080);

  add_tests_in_mode(true);
  add_tests_in_mode(false);

  add_test(function () {
    fakeOCSPResponder.stop(check_telemetry);
  });

  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX) {
  add_test(function () {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    run_next_test();
  });

  add_simple_tests(useMozillaPKIX);
  add_combo_tests(useMozillaPKIX);
  add_distrust_tests(useMozillaPKIX);

  add_test(function () {
    certOverrideService.clearValidityOverride("all:temporary-certificates", 0);
    run_next_test();
  });
}

function add_simple_tests(useMozillaPKIX) {
  add_cert_override_test("expired.example.com",
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_EXPIRED_CERTIFICATE));
  if (useMozillaPKIX) {
    add_cert_override_test("selfsigned.example.com",
                           Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                           getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  } else {
    add_non_overridable_test("selfsigned.example.com",
                             SEC_ERROR_CA_CERT_INVALID);
  }
  add_cert_override_test("unknownissuer.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  add_cert_override_test("expiredissuer.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                            useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                           : SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE));
  add_cert_override_test("md5signature.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                            SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED));
  add_cert_override_test("mismatch.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH,
                         getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_DOMAIN));

  
  
  
  
  
  
  
  
  
  if (useMozillaPKIX) {
    add_cert_override_test("selfsigned-inadequateEKU.example.com",
                           Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                           getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  } else {
    add_non_overridable_test("selfsigned-inadequateEKU.example.com",
                             SEC_ERROR_CA_CERT_INVALID);
  }

  
  
  if (useMozillaPKIX) {
    add_non_overridable_test("inadequatekeyusage.example.com",
                             SEC_ERROR_INADEQUATE_KEY_USAGE);
  } else {
    add_cert_override_test("inadequatekeyusage.example.com",
                           Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                           getXPCOMStatusFromNSS(SEC_ERROR_INADEQUATE_KEY_USAGE));
  }

  
  
  
  add_cert_override_test("self-signed-end-entity-with-cA-true.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                            useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                           : SEC_ERROR_UNTRUSTED_ISSUER));
}

function add_combo_tests(useMozillaPKIX) {
  
  

  add_cert_override_test("mismatch-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_DOMAIN));
  add_cert_override_test("mismatch-untrusted.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                            useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                           : SEC_ERROR_UNTRUSTED_ISSUER));
  add_cert_override_test("untrusted-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(
                            useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                           : SEC_ERROR_UNTRUSTED_ISSUER));
  add_cert_override_test("mismatch-untrusted-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(
                            useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                           : SEC_ERROR_UNTRUSTED_ISSUER));

  add_cert_override_test("md5signature-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(
                            SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED));
}

function add_distrust_tests(useMozillaPKIX) {
  
  add_connection_test("untrusted.example.com", Cr.NS_OK);

  
  
  add_distrust_override_test("tlsserver/default-ee.der",
                             "untrusted.example.com",
                             getXPCOMStatusFromNSS(SEC_ERROR_UNTRUSTED_CERT),
                             useMozillaPKIX
                                ? getXPCOMStatusFromNSS(SEC_ERROR_UNTRUSTED_CERT)
                                : Cr.NS_OK);

  
  
  add_distrust_override_test("tlsserver/other-test-ca.der",
                             "untrustedissuer.example.com",
                             getXPCOMStatusFromNSS(SEC_ERROR_UNTRUSTED_ISSUER),
                             useMozillaPKIX
                                ? getXPCOMStatusFromNSS(SEC_ERROR_UNTRUSTED_ISSUER)
                                : Cr.NS_OK);
}

function add_distrust_override_test(certFileName, hostName,
                                    expectedResultBefore, expectedResultAfter) {
  let certToDistrust = constructCertFromFile(certFileName);

  add_test(function () {
    
    setCertTrust(certToDistrust, "pu,,");
    clearSessionCache();
    run_next_test();
  });
  add_connection_test(hostName, expectedResultBefore, null,
                      function (securityInfo) {
                        securityInfo.QueryInterface(Ci.nsISSLStatusProvider);
                        
                        
                        if (securityInfo.SSLStatus) {
                          certOverrideService.rememberValidityOverride(
                              hostName, 8443, securityInfo.SSLStatus.serverCert,
                              Ci.nsICertOverrideService.ERROR_UNTRUSTED, true);
                        } else {
                          
                          
                          
                          
                          do_check_neq(expectedResultAfter, Cr.NS_OK);
                        }
                        clearSessionCache();
                      });
  add_connection_test(hostName, expectedResultAfter, null,
                      function () {
                        setCertTrust(certToDistrust, "u,,");
                      });
}
