




"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);



let certList = [
  
  'int-ev-valid',
  'ev-valid',

  
  'int-non-ev-root',
  'non-ev-root',
]

function load_ca(ca_name) {
  var ca_filename = ca_name + ".der";
  addCertFromFile(certdb, "test_ev_certs/" + ca_filename, 'CTu,CTu,CTu');
}

var gHttpServer;
var gOCSPResponseCounter = 0;

function start_ocsp_responder() {
  const SERVER_PORT = 8888;

  gHttpServer = new HttpServer();
  gHttpServer.registerPrefixHandler("/",
      function handleServerCallback(aRequest, aResponse) {
        let cert_nick = aRequest.path.slice(1, aRequest.path.length - 1);
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
        gOCSPResponseCounter++;
      });
  gHttpServer.identity.setPrimary("http", "www.example.com", SERVER_PORT);
  gHttpServer.start(SERVER_PORT);
}

function check_ee_for_ev(cert_name, expected_ev) {
    let cert = certdb.findCertByNickname(null, cert_name);
    let hasEVPolicy = {};
    let verifiedChain = {};
    let error = certdb.verifyCertNow(cert, certificateUsageSSLServer,
                                     0, verifiedChain, hasEVPolicy);
    if (isDebugBuild) {
      do_check_eq(hasEVPolicy.value, expected_ev);
    } else {
      do_check_false(hasEVPolicy.value);
    }
    do_check_eq(0, error);
}

function run_test() {
  for (let i = 0 ; i < certList.length; i++) {
    let cert_filename = certList[i] + ".der";
    addCertFromFile(certdb, "test_ev_certs/" + cert_filename, ',,');
  }
  load_ca("evroot");
  load_ca("non-evroot-ca");

  
  Services.prefs.setCharPref("network.dns.localDomains", 'www.example.com');
  start_ocsp_responder();

  run_next_test();
}


add_test(function() {
  check_ee_for_ev("ev-valid", true);
  run_next_test();
});

add_test(function() {
  check_ee_for_ev("non-ev-root", false);
  run_next_test();
});


add_test(function() {
  do_check_eq(4, gOCSPResponseCounter);
  gHttpServer.stop(run_next_test);
});

