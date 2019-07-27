


"use strict";




do_get_profile(); 
const certDB = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const SERVER_PORT = 8888;

function getOCSPResponder(expectedCertNames) {
  let expectedPaths = expectedCertNames.slice();
  return startOCSPResponder(SERVER_PORT, "www.example.com", [],
                            "test_keysize", expectedCertNames, expectedPaths);
}

function certFromFile(filename) {
  let der = readFile(do_get_file("test_keysize/" + filename, false));
  return certDB.constructX509(der, der.length);
}

function loadCert(certName, trustString) {
  let certFilename = certName + ".der";
  addCertFromFile(certDB, "test_keysize/" + certFilename, trustString);
  return certFromFile(certFilename);
}

function checkEVStatus(cert, usage, isEVExpected) {
  do_print("cert cn=" + cert.commonName);
  do_print("cert o=" + cert.organization);
  do_print("cert issuer cn=" + cert.issuerCommonName);
  do_print("cert issuer o=" + cert.issuerOrganization);
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certDB.verifyCertNow(cert, usage, NO_FLAGS, verifiedChain,
                                   hasEVPolicy);
  equal(hasEVPolicy.value, isEVExpected);
  equal(0, error);
}



















function addKeySizeTestForEV(expectedNamesForOCSP, certNamePrefix,
                             rootCACertFileName, subCACertFileNames,
                             endEntityCertFileName, expectedResult)
{
  add_test(function() {
    clearOCSPCache();
    let ocspResponder = getOCSPResponder(expectedNamesForOCSP);

    
    
    let rootCertNamePrefix = rootCACertFileName.startsWith("..")
                           ? ""
                           : certNamePrefix;
    loadCert(rootCertNamePrefix + rootCACertFileName, "CTu,CTu,CTu");
    for (let subCACertFileName of subCACertFileNames) {
      loadCert(certNamePrefix + subCACertFileName, ",,");
    }
    checkEVStatus(certFromFile(certNamePrefix + endEntityCertFileName + ".der"),
                  certificateUsageSSLServer, expectedResult);

    ocspResponder.stop(run_next_test);
  });
}















function checkForKeyType(keyType) {
  let certNamePrefix = "ev-" + keyType;

  
  let rootCAOKCertFileName = keyType == "rsa" ? "../test_ev_certs/evroot"
                                              : "-caOK";

  
  
  
  let expectedNamesForOCSP = isDebugBuild
                           ? [ certNamePrefix + "-intOK-caOK",
                               certNamePrefix + "-eeOK-intOK-caOK" ]
                           : [ certNamePrefix + "-eeOK-intOK-caOK" ];
  addKeySizeTestForEV(expectedNamesForOCSP, certNamePrefix,
                      rootCAOKCertFileName,
                      ["-intOK-caOK"],
                      "-eeOK-intOK-caOK",
                      isDebugBuild);

  
  
  expectedNamesForOCSP = [ certNamePrefix + "-eeOK-intOK-caBad" ];
  addKeySizeTestForEV(expectedNamesForOCSP, certNamePrefix,
                      "-caBad",
                      ["-intOK-caBad"],
                      "-eeOK-intOK-caBad",
                      false);

  
  
  expectedNamesForOCSP = isDebugBuild
                       ? [ certNamePrefix + "-intBad-caOK" ]
                       : [ certNamePrefix + "-eeOK-intBad-caOK" ];
  addKeySizeTestForEV(expectedNamesForOCSP, certNamePrefix,
                      rootCAOKCertFileName,
                      ["-intBad-caOK"],
                      "-eeOK-intBad-caOK",
                      false);

  
  
  expectedNamesForOCSP = [ certNamePrefix + "-eeBad-intOK-caOK" ];
  addKeySizeTestForEV(expectedNamesForOCSP, certNamePrefix,
                      rootCAOKCertFileName,
                      ["-intOK-caOK"],
                      "-eeBad-intOK-caOK",
                      false);
}

function run_test() {
  
  Services.prefs.setCharPref("network.dns.localDomains", "www.example.com");

  checkForKeyType("rsa");

  run_next_test();
}
