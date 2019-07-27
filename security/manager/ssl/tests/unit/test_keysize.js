



"use strict";





do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function certFromFile(filename) {
  let der = readFile(do_get_file("test_keysize/" + filename, false));
  return certdb.constructX509(der, der.length);
}

function loadCert(certName, trustString) {
  let certFilename = certName + ".der";
  addCertFromFile(certdb, "test_keysize/" + certFilename, trustString);
  return certFromFile(certFilename);
}














function checkChain(rootKeyType, rootKeySize, intKeyType, intKeySize,
                    eeKeyType, eeKeySize, eeExpectedError) {
  let rootName = "root_" + rootKeyType + "_" + rootKeySize;
  let intName = "int_" + intKeyType + "_" + intKeySize;
  let eeName = "ee_" + eeKeyType + "_" + eeKeySize;

  let intFullName = intName + "-" + rootName;
  let eeFullName = eeName + "-" + intName + "-" + rootName;

  loadCert(rootName, "CTu,CTu,CTu");
  loadCert(intFullName, ",,");
  let eeCert = certFromFile(eeFullName + ".der");

  do_print("cert cn=" + eeCert.commonName);
  do_print("cert o=" + eeCert.organization);
  do_print("cert issuer cn=" + eeCert.issuerCommonName);
  do_print("cert issuer o=" + eeCert.issuerOrganization);
  checkCertErrorGeneric(certdb, eeCert, eeExpectedError,
                        certificateUsageSSLServer);
}







function checkRSAChains(inadequateKeySize, adequateKeySize) {
  
  checkChain("rsa", adequateKeySize,
             "rsa", adequateKeySize,
             "rsa", adequateKeySize,
             PRErrorCodeSuccess);

  
  checkChain("rsa", inadequateKeySize,
             "rsa", adequateKeySize,
             "rsa", adequateKeySize,
             MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE);

  
  checkChain("rsa", adequateKeySize,
             "rsa", inadequateKeySize,
             "rsa", adequateKeySize,
             MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE);

  
  checkChain("rsa", adequateKeySize,
             "rsa", adequateKeySize,
             "rsa", inadequateKeySize,
             MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE);
}

function checkECCChains() {
  checkChain("prime256v1", 256,
             "secp384r1", 384,
             "secp521r1", 521,
             PRErrorCodeSuccess);
  checkChain("prime256v1", 256,
             "secp224r1", 224,
             "prime256v1", 256,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
  checkChain("prime256v1", 256,
             "prime256v1", 256,
             "secp224r1", 224,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
  checkChain("secp224r1", 224,
             "prime256v1", 256,
             "prime256v1", 256,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
  checkChain("prime256v1", 256,
             "prime256v1", 256,
             "secp256k1", 256,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
  checkChain("secp256k1", 256,
             "prime256v1", 256,
             "prime256v1", 256,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
}

function checkCombinationChains() {
  checkChain("rsa", 2048,
             "prime256v1", 256,
             "secp384r1", 384,
             PRErrorCodeSuccess);
  checkChain("rsa", 2048,
             "prime256v1", 256,
             "secp224r1", 224,
             SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
  checkChain("prime256v1", 256,
             "rsa", 1016,
             "prime256v1", 256,
             MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE);
}

function run_test() {
  checkRSAChains(1016, 1024);
  checkECCChains();
  checkCombinationChains();

  run_next_test();
}
