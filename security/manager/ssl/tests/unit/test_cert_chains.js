




"use strict";

function build_cert_chain(certNames) {
  let certList = Cc["@mozilla.org/security/x509certlist;1"]
                   .createInstance(Ci.nsIX509CertList);
  certNames.forEach(function(certName) {
    let cert = constructCertFromFile("tlsserver/" + certName + ".der");
    certList.addCert(cert);
  });
  return certList;
}

function test_cert_equals() {
  let certA = constructCertFromFile("tlsserver/default-ee.der");
  let certB = constructCertFromFile("tlsserver/default-ee.der");
  let certC = constructCertFromFile("tlsserver/expired-ee.der");

  do_check_false(certA == certB);
  do_check_true(certA.equals(certB));
  do_check_false(certA.equals(certC));
}

function test_cert_list_serialization() {
  let certList = build_cert_chain(['default-ee', 'expired-ee']);

  
  let serHelper = Cc["@mozilla.org/network/serialization-helper;1"]
                    .getService(Ci.nsISerializationHelper);
  certList.QueryInterface(Ci.nsISerializable);
  let serialized = serHelper.serializeToString(certList);

  
  let deserialized = serHelper.deserializeObject(serialized);
  deserialized.QueryInterface(Ci.nsIX509CertList);
  do_check_true(certList.equals(deserialized));
}

function run_test() {
  do_get_profile();
  add_tls_server_setup("BadCertServer");

  
  add_test(function() {
    test_cert_equals();
    run_next_test();
  });

  
  add_test(function() {
    test_cert_list_serialization();
    run_next_test();
  });

  
  add_connection_test(
    
    "good.include-subdomains.pinning.example.com", Cr.NS_OK, null,
    function withSecurityInfo(aTransportSecurityInfo) {
      aTransportSecurityInfo.QueryInterface(Ci.nsITransportSecurityInfo);
      do_check_eq(aTransportSecurityInfo.failedCertChain, null);
    }
  );

  
  add_connection_test(
    "expired.example.com",
    getXPCOMStatusFromNSS(SEC_ERROR_EXPIRED_CERTIFICATE),
    null,
    function withSecurityInfo(securityInfo) {
      securityInfo.QueryInterface(Ci.nsITransportSecurityInfo);
      do_check_neq(securityInfo.failedCertChain, null);
      let originalCertChain = build_cert_chain(["expired-ee", "test-ca"]);
      do_check_true(originalCertChain.equals(securityInfo.failedCertChain));
    }
  );

  
  add_connection_test(
    "inadequatekeyusage.example.com",
    getXPCOMStatusFromNSS(SEC_ERROR_INADEQUATE_KEY_USAGE),
    null,
    function withSecurityInfo(securityInfo) {
      securityInfo.QueryInterface(Ci.nsITransportSecurityInfo);
      do_check_neq(securityInfo.failedCertChain, null);
      let originalCertChain = build_cert_chain(["inadequatekeyusage-ee", "test-ca"]);
      do_check_true(originalCertChain.equals(securityInfo.failedCertChain));
    }
  );

  run_next_test();
}
