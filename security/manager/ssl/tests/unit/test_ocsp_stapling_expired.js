



"use strict";






let gCurrentOCSPResponse = null;
let gOCSPRequestCount = 0;

function add_ocsp_test(aHost, aExpectedResult, aOCSPResponseToServe) {
  add_connection_test(aHost, aExpectedResult,
    function() {
      clearOCSPCache();
      clearSessionCache();
      gCurrentOCSPResponse = aOCSPResponseToServe;
      gOCSPRequestCount = 0;
    },
    function() {
      do_check_eq(gOCSPRequestCount, 1);
    });
}

function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);
  let args = [ ["good", "localhostAndExampleCom", "unused" ],
               ["expiredresponse", "localhostAndExampleCom", "unused"],
               ["oldvalidperiod", "localhostAndExampleCom", "unused"],
               ["revoked", "localhostAndExampleCom", "unused"] ];
  let ocspResponses = generateOCSPResponses(args, "tlsserver");
  
  let ocspResponseGood = ocspResponses[0];
  
  let expiredOCSPResponseGood = ocspResponses[1];
  
  let oldValidityPeriodOCSPResponseGood = ocspResponses[2];
  
  let ocspResponseRevoked = ocspResponses[3];

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function(request, response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/ocsp-response");
    response.write(gCurrentOCSPResponse);
    gOCSPRequestCount++;
  });
  ocspResponder.start(8080);

  add_tls_server_setup("OCSPStaplingServer");

  
  
  
  
  
  
  
  
  
  add_ocsp_test("ocsp-stapling-expired.example.com", Cr.NS_OK,
                ocspResponseGood);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com", Cr.NS_OK,
                ocspResponseGood);
  add_ocsp_test("ocsp-stapling-expired.example.com", Cr.NS_OK,
                expiredOCSPResponseGood);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com", Cr.NS_OK,
                expiredOCSPResponseGood);
  add_ocsp_test("ocsp-stapling-expired.example.com", Cr.NS_OK,
                oldValidityPeriodOCSPResponseGood);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com", Cr.NS_OK,
                oldValidityPeriodOCSPResponseGood);
  add_ocsp_test("ocsp-stapling-expired.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE),
                ocspResponseRevoked);
  add_ocsp_test("ocsp-stapling-expired-fresh-ca.example.com",
                getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE),
                ocspResponseRevoked);
  add_test(function() { ocspResponder.stop(run_next_test); });
  add_test(check_ocsp_stapling_telemetry);
  run_next_test();
}

function check_ocsp_stapling_telemetry() {
  let histogram = Cc["@mozilla.org/base/telemetry;1"]
                    .getService(Ci.nsITelemetry)
                    .getHistogramById("SSL_OCSP_STAPLING")
                    .snapshot();
  do_check_eq(histogram.counts[0], 0); 
  do_check_eq(histogram.counts[1], 0); 
  do_check_eq(histogram.counts[2], 0); 
  do_check_eq(histogram.counts[3], 8); 
  do_check_eq(histogram.counts[4], 0); 
  run_next_test();
}
