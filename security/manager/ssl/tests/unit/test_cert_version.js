




























"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function certFromFile(certName) {
  return constructCertFromFile("test_cert_version/" + certName + ".pem");
}

function loadCertWithTrust(certName, trustString) {
  addCertFromFile(certdb, "test_cert_version/" + certName + ".pem", trustString);
}

function checkEndEntity(cert, expectedResult) {
  checkCertErrorGeneric(certdb, cert, expectedResult, certificateUsageSSLServer);
}

function checkIntermediate(cert, expectedResult) {
  checkCertErrorGeneric(certdb, cert, expectedResult, certificateUsageSSLCA);
}

function run_test() {
  loadCertWithTrust("ca", "CTu,,");

  
  loadCertWithTrust("int-v1-noBC_ca", ",,");
  checkIntermediate(certFromFile("int-v1-noBC_ca"), MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA);
  checkEndEntity(certFromFile("ee_int-v1-noBC"), MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA);
  
  
  loadCertWithTrust("int-v1-noBC_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v1-noBC_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v1-noBC"), PRErrorCodeSuccess);

  loadCertWithTrust("int-v2-noBC_ca", ",,");
  checkIntermediate(certFromFile("int-v2-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v2-noBC"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v2-noBC_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v2-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v2-noBC"), SEC_ERROR_CA_CERT_INVALID);

  loadCertWithTrust("int-v3-noBC_ca", ",,");
  checkIntermediate(certFromFile("int-v3-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v3-noBC"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v3-noBC_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v3-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v3-noBC"), SEC_ERROR_CA_CERT_INVALID);

  loadCertWithTrust("int-v4-noBC_ca", ",,");
  checkIntermediate(certFromFile("int-v4-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v4-noBC"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v4-noBC_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v4-noBC_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v4-noBC"), SEC_ERROR_CA_CERT_INVALID);

  
  loadCertWithTrust("int-v1-BC-not-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v1-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v1-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v1-BC-not-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v1-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v1-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);

  loadCertWithTrust("int-v2-BC-not-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v2-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v2-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v2-BC-not-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v2-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v2-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);

  loadCertWithTrust("int-v3-BC-not-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v3-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v3-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v3-BC-not-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v3-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v3-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);

  loadCertWithTrust("int-v4-BC-not-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v4-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v4-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);
  loadCertWithTrust("int-v4-BC-not-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v4-BC-not-cA_ca"), SEC_ERROR_CA_CERT_INVALID);
  checkEndEntity(certFromFile("ee_int-v4-BC-not-cA"), SEC_ERROR_CA_CERT_INVALID);

  
  loadCertWithTrust("int-v1-BC-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v1-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v1-BC-cA"), PRErrorCodeSuccess);
  loadCertWithTrust("int-v1-BC-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v1-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v1-BC-cA"), PRErrorCodeSuccess);

  loadCertWithTrust("int-v2-BC-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v2-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v2-BC-cA"), PRErrorCodeSuccess);
  loadCertWithTrust("int-v2-BC-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v2-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v2-BC-cA"), PRErrorCodeSuccess);

  loadCertWithTrust("int-v3-BC-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v3-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v3-BC-cA"), PRErrorCodeSuccess);
  loadCertWithTrust("int-v3-BC-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v3-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v3-BC-cA"), PRErrorCodeSuccess);

  loadCertWithTrust("int-v4-BC-cA_ca", ",,");
  checkIntermediate(certFromFile("int-v4-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v4-BC-cA"), PRErrorCodeSuccess);
  loadCertWithTrust("int-v4-BC-cA_ca", "CTu,,");
  checkIntermediate(certFromFile("int-v4-BC-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee_int-v4-BC-cA"), PRErrorCodeSuccess);

  
  checkEndEntity(certFromFile("ee-v1-noBC_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v2-noBC_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v3-noBC_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v4-noBC_ca"), PRErrorCodeSuccess);

  checkEndEntity(certFromFile("ee-v1-BC-not-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v2-BC-not-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v3-BC-not-cA_ca"), PRErrorCodeSuccess);
  checkEndEntity(certFromFile("ee-v4-BC-not-cA_ca"), PRErrorCodeSuccess);

  checkEndEntity(certFromFile("ee-v1-BC-cA_ca"), MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY);
  checkEndEntity(certFromFile("ee-v2-BC-cA_ca"), MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY);
  checkEndEntity(certFromFile("ee-v3-BC-cA_ca"), MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY);
  checkEndEntity(certFromFile("ee-v4-BC-cA_ca"), MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY);

  
  checkEndEntity(certFromFile("ss-v1-noBC"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v2-noBC"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v3-noBC"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v4-noBC"), SEC_ERROR_UNKNOWN_ISSUER);

  checkEndEntity(certFromFile("ss-v1-BC-not-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v2-BC-not-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v3-BC-not-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v4-BC-not-cA"), SEC_ERROR_UNKNOWN_ISSUER);

  checkEndEntity(certFromFile("ss-v1-BC-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v2-BC-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v3-BC-cA"), SEC_ERROR_UNKNOWN_ISSUER);
  checkEndEntity(certFromFile("ss-v4-BC-cA"), SEC_ERROR_UNKNOWN_ISSUER);
}
