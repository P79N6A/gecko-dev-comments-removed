


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
















function addKeySizeTestForEV(expectedNamesForOCSP,
                             rootCertFileName, intCertFileNames,
                             endEntityCertFileName, expectedResult)
{
  add_test(function() {
    clearOCSPCache();
    let ocspResponder = getOCSPResponder(expectedNamesForOCSP);

    loadCert(rootCertFileName, "CTu,CTu,CTu");
    for (let intCertFileName of intCertFileNames) {
      loadCert(intCertFileName, ",,");
    }
    checkEVStatus(certFromFile(endEntityCertFileName + ".der"),
                  certificateUsageSSLServer, expectedResult);

    ocspResponder.stop(run_next_test);
  });
}

















function checkRSAChains(inadequateKeySize, adequateKeySize) {
  
  let rootOKCertFileName = "../test_ev_certs/evroot";
  let rootOKName = "evroot";
  let rootNotOKName = "ev_root_rsa_" + inadequateKeySize;
  let intOKName = "ev_int_rsa_" + adequateKeySize;
  let intNotOKName = "ev_int_rsa_" + inadequateKeySize;
  let eeOKName = "ev_ee_rsa_" + adequateKeySize;
  let eeNotOKName = "ev_ee_rsa_" + inadequateKeySize;

  
  
  
  
  let intFullName = intOKName + "-" + rootOKName;
  let eeFullName = eeOKName + "-" + intOKName + "-" + rootOKName;
  let expectedNamesForOCSP = gEVExpected
                           ? [ intFullName,
                               eeFullName ]
                           : [ eeFullName ];
  addKeySizeTestForEV(expectedNamesForOCSP, rootOKCertFileName,
                      [ intFullName ], eeFullName, gEVExpected);

  
  
  intFullName = intOKName + "-" + rootNotOKName;
  eeFullName = eeOKName + "-" + intOKName + "-" + rootNotOKName;
  expectedNamesForOCSP = [ eeFullName ];
  addKeySizeTestForEV(expectedNamesForOCSP, rootNotOKName,
                      [ intFullName ], eeFullName, false);

  
  
  intFullName = intNotOKName + "-" + rootOKName;
  eeFullName = eeOKName + "-" + intNotOKName + "-" + rootOKName;
  expectedNamesForOCSP = [ eeFullName ];
  addKeySizeTestForEV(expectedNamesForOCSP, rootOKCertFileName,
                      [ intFullName ], eeFullName, false);

  
  
  intFullName = intOKName + "-" + rootOKName;
  eeFullName = eeNotOKName + "-" + intOKName + "-" + rootOKName;
  expectedNamesForOCSP = gEVExpected
                       ? [ intFullName,
                           eeFullName ]
                       : [ eeFullName ];
  addKeySizeTestForEV(expectedNamesForOCSP, rootOKCertFileName,
                      [ intFullName ], eeFullName, false);
}

function run_test() {
  Services.prefs.setCharPref("network.dns.localDomains", "www.example.com");

  checkRSAChains(2040, 2048);

  run_next_test();
}
