


"use strict";




do_get_profile(); 
const certDB = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const SERVER_PORT = 8888;

function getOCSPResponder(expectedCertNames) {
  let expectedPaths = expectedCertNames.slice();
  return startOCSPResponder(SERVER_PORT, "www.example.com", [],
                            "test_validity", expectedCertNames, expectedPaths);
}

function certFromFile(filename) {
  return constructCertFromFile(`test_validity/${filename}`);
}

function loadCert(certFilename, trustString) {
  addCertFromFile(certDB, `test_validity/${certFilename}`, trustString);
}
















function addEVTest(expectedNamesForOCSP, rootCertFileName, intCertFileNames,
                   endEntityCertFileName, expectedResult)
{
  add_test(function() {
    clearOCSPCache();
    let ocspResponder = getOCSPResponder(expectedNamesForOCSP);

    loadCert(`${rootCertFileName}.der`, "CTu,CTu,CTu");
    for (let intCertFileName of intCertFileNames) {
      loadCert(`${intCertFileName}.der`, ",,");
    }
    checkEVStatus(certDB, certFromFile(`${endEntityCertFileName}.der`),
                  certificateUsageSSLServer, expectedResult);

    ocspResponder.stop(run_next_test);
  });
}

function checkEVChains() {
  
  
  const intFullName = "ev_int_60_months-evroot";
  let eeFullName = `ev_ee_39_months-${intFullName}`;
  let expectedNamesForOCSP = gEVExpected
                           ? [ intFullName,
                               eeFullName ]
                           : [ eeFullName ];
  addEVTest(expectedNamesForOCSP, "../test_ev_certs/evroot", [ intFullName ],
            eeFullName, gEVExpected);

  
  
  eeFullName = `ev_ee_40_months-${intFullName}`;
  expectedNamesForOCSP = gEVExpected
                           ? [ intFullName,
                               eeFullName ]
                           : [ eeFullName ];
  addEVTest(expectedNamesForOCSP, "../test_ev_certs/evroot", [ intFullName ],
            eeFullName, false);
}

function run_test() {
  Services.prefs.setCharPref("network.dns.localDomains", "www.example.com");
  Services.prefs.setIntPref("security.OCSP.enabled", 1);

  checkEVChains();

  run_next_test();
}
