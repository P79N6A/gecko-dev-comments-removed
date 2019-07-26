



"use strict";






function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.OCSP.require", true);

  let args = [ ["bad-signature", "localhostAndExampleCom", "unused" ] ];
  let ocspResponses = generateOCSPResponses(args, "tlsserver");
  let ocspResponseBadSignature = ocspResponses[0];
  let ocspRequestCount = 0;

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function(request, response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/ocsp-response");
    response.write(ocspResponseBadSignature);
    ocspRequestCount++;
  });
  ocspResponder.start(8080);

  
  
  add_tls_server_setup("OCSPStaplingServer");
  add_connection_test("ocsp-stapling-none.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT));
  
  
  
  
  add_connection_test("ocsp-stapling-none.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_OCSP_INVALID_SIGNING_CERT));
  add_test(function() { ocspResponder.stop(run_next_test); });
  add_test(function() { do_check_eq(ocspRequestCount, 1); run_next_test(); });
  run_next_test();
}
