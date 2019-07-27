




"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const evrootnick = "XPCShell EV Testing (untrustworthy) CA - Mozilla - " +
                   "EV debug test CA";



let certList = [
  
  'int-ev-valid',
  'ev-valid',
  'ev-valid-anypolicy-int',
  'int-ev-valid-anypolicy-int',
  'no-ocsp-url-cert', 
                      

  
  'int-non-ev-root',
  'non-ev-root',
];

function load_ca(ca_name) {
  var ca_filename = ca_name + ".der";
  addCertFromFile(certdb, "test_ev_certs/" + ca_filename, 'CTu,CTu,CTu');
}

const SERVER_PORT = 8888;

function failingOCSPResponder() {
  return getFailingHttpServer(SERVER_PORT,
                              ["www.example.com", "crl.example.com"]);
}

function start_ocsp_responder(expectedCertNames) {
  let expectedPaths = expectedCertNames.slice();
  return startOCSPResponder(SERVER_PORT, "www.example.com", ["crl.example.com"],
                            "test_ev_certs", expectedCertNames, expectedPaths);
}

function check_cert_err(cert_name, expected_error) {
  let cert = certdb.findCertByNickname(null, cert_name);
  checkCertErrorGeneric(certdb, cert, expected_error, certificateUsageSSLServer);
}


function check_ee_for_ev(cert_name, expected_ev) {
  let cert = certdb.findCertByNickname(null, cert_name);
  checkEVStatus(certdb, cert, certificateUsageSSLServer, expected_ev);
}

function run_test() {
  for (let i = 0 ; i < certList.length; i++) {
    let cert_filename = certList[i] + ".der";
    addCertFromFile(certdb, "test_ev_certs/" + cert_filename, ',,');
  }
  load_ca("evroot");
  load_ca("non-evroot-ca");

  
  Services.prefs.setCharPref("network.dns.localDomains",
                             'www.example.com, crl.example.com');
  Services.prefs.setIntPref("security.OCSP.enabled", 1);

  add_test(function () {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    ocspResponder.stop(run_next_test);
  });


  add_test(function () {
    clearOCSPCache();

    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid-anypolicy-int", "ev-valid-anypolicy-int"]
                                      : ["ev-valid-anypolicy-int"]);
    check_ee_for_ev("ev-valid-anypolicy-int", gEVExpected);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(["non-ev-root"]);
    check_ee_for_ev("non-ev-root", false);
    ocspResponder.stop(run_next_test);
  });

  add_test(function() {
    clearOCSPCache();
    let ocspResponder = gEVExpected ? start_ocsp_responder(["int-ev-valid"])
                                    : failingOCSPResponder();
    check_ee_for_ev("no-ocsp-url-cert", false);
    ocspResponder.stop(run_next_test);
  });

  
  const nsIX509Cert = Ci.nsIX509Cert;
  add_test(function() {
    let evRootCA = certdb.findCertByNickname(null, evrootnick);
    certdb.setCertTrust(evRootCA, nsIX509Cert.CA_CERT, 0);

    clearOCSPCache();
    let ocspResponder = failingOCSPResponder();
    check_cert_err("ev-valid",SEC_ERROR_UNKNOWN_ISSUER);
    ocspResponder.stop(run_next_test);
  });

  
  
  add_test(function() {
    let evRootCA = certdb.findCertByNickname(null, evrootnick);
    certdb.setCertTrust(evRootCA, nsIX509Cert.CA_CERT,
                        Ci.nsIX509CertDB.TRUSTED_SSL |
                        Ci.nsIX509CertDB.TRUSTED_EMAIL |
                        Ci.nsIX509CertDB.TRUSTED_OBJSIGN);

    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    ocspResponder.stop(run_next_test);
  });

  add_test(function () {
    check_no_ocsp_requests("ev-valid", SEC_ERROR_POLICY_VALIDATION_FAILED);
  });

  add_test(function () {
    check_no_ocsp_requests("non-ev-root", SEC_ERROR_POLICY_VALIDATION_FAILED);
  });

  add_test(function () {
    check_no_ocsp_requests("no-ocsp-url-cert", SEC_ERROR_POLICY_VALIDATION_FAILED);
  });

  
  add_test(function () {
    
    Services.prefs.setIntPref("security.onecrl.maximum_staleness_in_seconds", 86400);
    
    Services.prefs.setIntPref("app.update.lastUpdateTime.blocklist-background-update-timer",
                              Math.floor(Date.now() / 1000) - 1);
    clearOCSPCache();
    
    let ocspResponder = start_ocsp_responder(["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    Services.prefs.clearUserPref("security.onecrl.maximum_staleness_in_seconds");
    ocspResponder.stop(run_next_test);
  });

  add_test(function () {
    
    Services.prefs.setIntPref("security.onecrl.maximum_staleness_in_seconds", 0);
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    Services.prefs.clearUserPref("security.onecrl.maximum_staleness_in_seconds");
    ocspResponder.stop(run_next_test);
  });

  add_test(function () {
    
    Services.prefs.setIntPref("security.onecrl.maximum_staleness_in_seconds", 86400);
    
    Services.prefs.setIntPref("app.update.lastUpdateTime.blocklist-background-update-timer",
                              Math.floor(Date.now() / 1000) - 86480);
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    Services.prefs.clearUserPref("security.onecrl.maximum_staleness_in_seconds");
    ocspResponder.stop(run_next_test);
  });

  
  add_test(function () {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    ocspResponder.stop(function () {
      
      let failingOcspResponder = failingOCSPResponder();
      let cert = certdb.findCertByNickname(null, "ev-valid");
      let hasEVPolicy = {};
      let verifiedChain = {};
      let flags = Ci.nsIX509CertDB.FLAG_LOCAL_ONLY |
                  Ci.nsIX509CertDB.FLAG_MUST_BE_EV;

      let error = certdb.verifyCertNow(cert, certificateUsageSSLServer, flags,
                                       null, verifiedChain, hasEVPolicy);
      do_check_eq(hasEVPolicy.value, gEVExpected);
      do_check_eq(error,
                  gEVExpected ? PRErrorCodeSuccess
                              : SEC_ERROR_POLICY_VALIDATION_FAILED);
      failingOcspResponder.stop(run_next_test);
    });
  });

  
  add_test(function () {
    clearOCSPCache();
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          gEVExpected ? ["int-ev-valid", "ev-valid"]
                                      : ["ev-valid"],
                          [], [],
                          gEVExpected ? ["longvalidityalmostold", "good"]
                                      : ["good"]);
    check_ee_for_ev("ev-valid", gEVExpected);
    ocspResponder.stop(run_next_test);
  });

  
  
  
  add_test(function () {
    clearOCSPCache();
    
    
    
    let debugCertNickArray = ["int-ev-valid", "ev-valid", "ev-valid"];
    let debugResponseArray = ["good", "longvalidityalmostold",
                              "longvalidityalmostold"];
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          gEVExpected ? debugCertNickArray : ["ev-valid"],
                          [], [],
                          gEVExpected ? debugResponseArray
                                      : ["longvalidityalmostold"]);
    check_ee_for_ev("ev-valid", false);
    ocspResponder.stop(run_next_test);
  });

  
  
  add_test(function () {
    clearOCSPCache();
    let debugCertNickArray = ["int-ev-valid", "ev-valid", "ev-valid"];
    let debugResponseArray = ["good", "ancientstillvalid",
                              "ancientstillvalid"];
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          gEVExpected ? debugCertNickArray : ["ev-valid"],
                          [], [],
                          gEVExpected ? debugResponseArray
                                      : ["ancientstillvalid"]);
    check_ee_for_ev("ev-valid", false);
    ocspResponder.stop(run_next_test);
  });

  run_next_test();
}








function check_no_ocsp_requests(cert_name, expected_error) {
  clearOCSPCache();
  let ocspResponder = failingOCSPResponder();
  let cert = certdb.findCertByNickname(null, cert_name);
  let hasEVPolicy = {};
  let verifiedChain = {};
  let flags = Ci.nsIX509CertDB.FLAG_LOCAL_ONLY |
              Ci.nsIX509CertDB.FLAG_MUST_BE_EV;
  let error = certdb.verifyCertNow(cert, certificateUsageSSLServer, flags,
                                   null, verifiedChain, hasEVPolicy);
  
  do_check_eq(hasEVPolicy.value, false);
  do_check_eq(expected_error, error);
  ocspResponder.stop(run_next_test);
}
