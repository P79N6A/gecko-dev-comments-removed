




"use strict";




do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

const evrootnick = "XPCShell EV Testing (untrustworthy) CA - Mozilla - " +
                   "EV debug test CA";



let certList = [
  
  'int-ev-valid',
  'ev-valid',
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
  let httpServer = new HttpServer();
  httpServer.registerPrefixHandler("/", function(request, response) {
    do_check_true(false);
  });
  httpServer.identity.setPrimary("http", "www.example.com", SERVER_PORT);
  httpServer.identity.add("http", "crl.example.com", SERVER_PORT);
  httpServer.start(SERVER_PORT);
  return httpServer;
}

function start_ocsp_responder(expectedCertNames) {
  let httpServer = new HttpServer();
  httpServer.registerPrefixHandler("/",
      function handleServerCallback(aRequest, aResponse) {
        do_check_neq(aRequest.host, "crl.example.com"); 
        let cert_nick = aRequest.path.slice(1, aRequest.path.length - 1);

        do_check_true(expectedCertNames.length >= 1);
        let expected_nick = expectedCertNames.shift();
        do_check_eq(cert_nick, expected_nick);

        do_print("Generating ocsp response for '" + cert_nick + "'");
        aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
        aResponse.setHeader("Content-Type", "application/ocsp-response");
        
        let ocsp_request_desc = new Array();
        ocsp_request_desc.push("good");
        ocsp_request_desc.push(cert_nick);
        ocsp_request_desc.push("unused_arg");
        let arg_array = new Array();
        arg_array.push(ocsp_request_desc);
        let retArray = generateOCSPResponses(arg_array, "test_ev_certs");
        let responseBody = retArray[0];
        aResponse.bodyOutputStream.write(responseBody, responseBody.length);
      });
  httpServer.identity.setPrimary("http", "www.example.com", SERVER_PORT);
  httpServer.identity.add("http", "crl.example.com", SERVER_PORT);
  httpServer.start(SERVER_PORT);
  return {
    stop: function(callback) {
      do_check_eq(expectedCertNames.length, 0);
      httpServer.stop(callback);
    }
  };
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

  run_next_test();
}

add_test(function() {
  clearOCSPCache();
  let ocspResponder = start_ocsp_responder(
                        isDebugBuild ? ["int-ev-valid", "ev-valid"]
                                     : ["ev-valid"]);
  check_ee_for_ev("ev-valid", isDebugBuild);
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
  check_cert_err("ev-valid", SEC_ERROR_UNTRUSTED_ISSUER);
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
