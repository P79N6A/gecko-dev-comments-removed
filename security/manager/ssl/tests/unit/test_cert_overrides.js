



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
  do_check_eq(histogram.counts[ 2], 7); 
  do_check_eq(histogram.counts[ 3], 0); 
  do_check_eq(histogram.counts[ 4], 0); 
  do_check_eq(histogram.counts[ 5], 1); 
  do_check_eq(histogram.counts[ 6], 0); 
  do_check_eq(histogram.counts[ 7], 0); 
  do_check_eq(histogram.counts[ 8], 2); 
  do_check_eq(histogram.counts[ 9], 6); 
  do_check_eq(histogram.counts[10], 5); 
  do_check_eq(histogram.counts[11], 2); 
  do_check_eq(histogram.counts[12], 1); 
  do_check_eq(histogram.counts[13], 1); 
  do_check_eq(histogram.counts[14], 2); 
  do_check_eq(histogram.counts[15], 1); 
  do_check_eq(histogram.counts[16], 2); 
  run_next_test();
}

function run_test() {
  add_tls_server_setup("BadCertServer");

  let fakeOCSPResponder = new HttpServer();
  fakeOCSPResponder.registerPrefixHandler("/", function (request, response) {
    response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
  });
  fakeOCSPResponder.start(8888);

  add_simple_tests();
  add_combo_tests();
  add_distrust_tests();

  add_test(function () {
    fakeOCSPResponder.stop(check_telemetry);
  });

  run_next_test();
}

function add_simple_tests() {
  add_cert_override_test("expired.example.com",
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_EXPIRED_CERTIFICATE));
  add_cert_override_test("notyetvalid.example.com",
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(
                           MOZILLA_PKIX_ERROR_NOT_YET_VALID_CERTIFICATE));
  add_cert_override_test("before-epoch.example.com",
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_INVALID_TIME));
  add_cert_override_test("selfsigned.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  add_cert_override_test("unknownissuer.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  add_cert_override_test("expiredissuer.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE));
  add_cert_override_test("notyetvalidissuer.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                           MOZILLA_PKIX_ERROR_NOT_YET_VALID_ISSUER_CERTIFICATE));
  add_cert_override_test("before-epoch-issuer.example.com",
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_INVALID_TIME));
  add_cert_override_test("md5signature.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(
                            SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED));
  add_cert_override_test("mismatch.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH,
                         getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_DOMAIN));

  
  
  
  add_cert_override_test("selfsigned-inadequateEKU.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));

  add_non_overridable_test("inadequatekeyusage.example.com",
                           SEC_ERROR_INADEQUATE_KEY_USAGE);

  
  
  
  
  add_test(function() {
    let rootCert = constructCertFromFile("tlsserver/test-ca.der");
    setCertTrust(rootCert, ",,");
    run_next_test();
  });
  add_non_overridable_test("badSubjectAltNames.example.com", SEC_ERROR_BAD_DER);
  add_test(function() {
    let rootCert = constructCertFromFile("tlsserver/test-ca.der");
    setCertTrust(rootCert, "CTu,,");
    run_next_test();
  });

  
  
  
  add_cert_override_test("self-signed-end-entity-with-cA-true.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));

  add_cert_override_test("ca-used-as-end-entity.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY));

  
  
  add_cert_override_test("end-entity-issued-by-v1-cert.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA));
  
  add_test(function() {
    certOverrideService.clearValidityOverride("end-entity-issued-by-v1-cert.example.com", 8443);
    let v1Cert = constructCertFromFile("tlsserver/v1Cert.der");
    setCertTrust(v1Cert, "CTu,,");
    clearSessionCache();
    run_next_test();
  });
  add_connection_test("end-entity-issued-by-v1-cert.example.com", Cr.NS_OK);
  
  add_test(function() {
    let v1Cert = constructCertFromFile("tlsserver/v1Cert.der");
    setCertTrust(v1Cert, ",,");
    clearSessionCache();
    run_next_test();
  });

  add_cert_override_test("inadequate-key-size-ee.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE));
}

function add_combo_tests() {
  add_cert_override_test("mismatch-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_DOMAIN));
  add_cert_override_test("mismatch-notYetValid.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SSL_ERROR_BAD_CERT_DOMAIN));
  add_cert_override_test("mismatch-untrusted.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  add_cert_override_test("untrusted-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
  add_cert_override_test("mismatch-untrusted-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));

  add_cert_override_test("md5signature-expired.example.com",
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                         Ci.nsICertOverrideService.ERROR_TIME,
                         getXPCOMStatusFromNSS(
                            SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED));

  add_cert_override_test("ca-used-as-end-entity-name-mismatch.example.com",
                         Ci.nsICertOverrideService.ERROR_MISMATCH |
                         Ci.nsICertOverrideService.ERROR_UNTRUSTED,
                         getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY));
}

function add_distrust_tests() {
  
  add_connection_test("untrusted.example.com", Cr.NS_OK);

  add_distrust_test("tlsserver/default-ee.der", "untrusted.example.com",
                    SEC_ERROR_UNTRUSTED_CERT);

  add_distrust_test("tlsserver/other-test-ca.der",
                    "untrustedissuer.example.com", SEC_ERROR_UNTRUSTED_ISSUER);

  add_distrust_test("tlsserver/test-ca.der",
                    "ca-used-as-end-entity.example.com",
                    SEC_ERROR_UNTRUSTED_ISSUER);
}

function add_distrust_test(certFileName, hostName, expectedResult) {
  let certToDistrust = constructCertFromFile(certFileName);

  add_test(function () {
    
    setCertTrust(certToDistrust, "pu,,");
    clearSessionCache();
    run_next_test();
  });
  add_non_overridable_test(hostName, expectedResult);
  add_test(function () {
    setCertTrust(certToDistrust, "u,,");
    run_next_test();
  });
}
