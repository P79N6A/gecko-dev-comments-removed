




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
]

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
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certdb.verifyCertNow(cert, certificateUsageSSLServer,
                                   NO_FLAGS, verifiedChain, hasEVPolicy);
  do_check_eq(error,  expected_error);
}


function check_ee_for_ev(cert_name, expected_ev) {
    let cert = certdb.findCertByNickname(null, cert_name);
    let hasEVPolicy = {};
    let verifiedChain = {};
    let error = certdb.verifyCertNow(cert, certificateUsageSSLServer,
                                     NO_FLAGS, verifiedChain, hasEVPolicy);
    do_check_eq(hasEVPolicy.value, expected_ev);
    do_check_eq(0, error);
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
  add_tests_in_mode(true);
  add_tests_in_mode(false);
  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX)
{
  add_test(function () {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    run_next_test();
  });

  add_test(function () {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          isDebugBuild ? ["int-ev-valid", "ev-valid"]
                                       : ["ev-valid"]);
    check_ee_for_ev("ev-valid", isDebugBuild);
    ocspResponder.stop(run_next_test);
  });


  add_test(function () {
    clearOCSPCache();

    let ocspResponder = start_ocsp_responder(
                          isDebugBuild ? ["int-ev-valid-anypolicy-int", "ev-valid-anypolicy-int"]
                                       : ["ev-valid-anypolicy-int"]);
    check_ee_for_ev("ev-valid-anypolicy-int", isDebugBuild);
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
    
    
    let ocspResponder = isDebugBuild ? start_ocsp_responder(["int-ev-valid"])
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
    check_cert_err("ev-valid",
                   useMozillaPKIX ? SEC_ERROR_UNKNOWN_ISSUER
                                  : SEC_ERROR_UNTRUSTED_ISSUER);
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
                          isDebugBuild ? ["int-ev-valid", "ev-valid"]
                                       : ["ev-valid"]);
    check_ee_for_ev("ev-valid", isDebugBuild);
    ocspResponder.stop(run_next_test);
  });

  add_test(function () {
    check_no_ocsp_requests("ev-valid",
      useMozillaPKIX ? SEC_ERROR_POLICY_VALIDATION_FAILED
                     : (isDebugBuild ? SEC_ERROR_REVOKED_CERTIFICATE
                                     : SEC_ERROR_EXTENSION_NOT_FOUND));
  });

  add_test(function () {
    check_no_ocsp_requests("non-ev-root",
      useMozillaPKIX ? SEC_ERROR_POLICY_VALIDATION_FAILED
                     : (isDebugBuild ? SEC_ERROR_UNTRUSTED_ISSUER
                                     : SEC_ERROR_EXTENSION_NOT_FOUND));
  });

  add_test(function () {
    check_no_ocsp_requests("no-ocsp-url-cert",
      useMozillaPKIX ? SEC_ERROR_POLICY_VALIDATION_FAILED
                     : (isDebugBuild ? SEC_ERROR_REVOKED_CERTIFICATE
                                     : SEC_ERROR_EXTENSION_NOT_FOUND));
  });


  
  add_test(function () {
    clearOCSPCache();
    let ocspResponder = start_ocsp_responder(
                          isDebugBuild ? ["int-ev-valid", "ev-valid"]
                                       : ["ev-valid"]);
    check_ee_for_ev("ev-valid", isDebugBuild);
    ocspResponder.stop(function () {
      
      let failingOcspResponder = failingOCSPResponder();
      let cert = certdb.findCertByNickname(null, "ev-valid");
      let hasEVPolicy = {};
      let verifiedChain = {};
      let flags = Ci.nsIX509CertDB.FLAG_LOCAL_ONLY |
                  Ci.nsIX509CertDB.FLAG_MUST_BE_EV;

      let error = certdb.verifyCertNow(cert, certificateUsageSSLServer,
                                       flags, verifiedChain, hasEVPolicy);
      do_check_eq(hasEVPolicy.value, isDebugBuild);
      do_check_eq(error,
                  isDebugBuild ? 0
                               : (useMozillaPKIX ? SEC_ERROR_POLICY_VALIDATION_FAILED
                                                 : SEC_ERROR_EXTENSION_NOT_FOUND));
      failingOcspResponder.stop(run_next_test);
    });
  });

  
  add_test(function () {
    clearOCSPCache();
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          isDebugBuild ? ["int-ev-valid", "ev-valid"]
                                       : ["ev-valid"],
                          [], [],
                          isDebugBuild ? ["longvalidityalmostold", "good"]
                                       : ["good"]);
    check_ee_for_ev("ev-valid", isDebugBuild);
    ocspResponder.stop(run_next_test);
  });

  
  
  
  add_test(function () {
    clearOCSPCache();
    
    
    
    let debugCertNickArray = ["int-ev-valid", "ev-valid", "ev-valid"];
    let debugResponseArray = ["good", "longvalidityalmostold",
                              "longvalidityalmostold"];
    if (!useMozillaPKIX) {
      debugCertNickArray = ["int-ev-valid", "ev-valid"];
      debugResponseArray = ["good", "longvalidityalmostold"];
    }
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          isDebugBuild ? debugCertNickArray : ["ev-valid"],
                          [], [],
                          isDebugBuild ? debugResponseArray
                                       : ["longvalidityalmostold"]);
    check_ee_for_ev("ev-valid", !useMozillaPKIX && isDebugBuild);
    ocspResponder.stop(run_next_test);
  });

  
  
  add_test(function () {
    clearOCSPCache();
    let debugCertNickArray = ["int-ev-valid", "ev-valid", "ev-valid"];
    let debugResponseArray = ["good", "ancientstillvalid",
                              "ancientstillvalid"];
    if (!useMozillaPKIX) {
      debugCertNickArray = ["int-ev-valid", "ev-valid"];
      debugResponseArray = ["good", "ancientstillvalid"];
    }
    let ocspResponder = startOCSPResponder(SERVER_PORT, "www.example.com", [],
                          "test_ev_certs",
                          isDebugBuild ? debugCertNickArray : ["ev-valid"],
                          [], [],
                          isDebugBuild ? debugResponseArray
                                       : ["ancientstillvalid"]);
    check_ee_for_ev("ev-valid", !useMozillaPKIX && isDebugBuild);
    ocspResponder.stop(run_next_test);
  });
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
                                   verifiedChain, hasEVPolicy);
  
  do_check_eq(hasEVPolicy.value, false);
  do_check_eq(expected_error, error);
  
  let identityInfo = cert.QueryInterface(Ci.nsIIdentityInfo);
  do_check_eq(identityInfo.isExtendedValidation, false);
  ocspResponder.stop(run_next_test);
}

