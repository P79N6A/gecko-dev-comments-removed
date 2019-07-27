



"use strict";




function run_test() {
  do_get_profile();
  
  Services.prefs.setBoolPref("security.OCSP.require", true);

  
  
  add_tls_server_setup("OCSPStaplingServer");

  let args = [["good", "localhostAndExampleCom", "unused"]];
  let ocspResponses = generateOCSPResponses(args, "tlsserver");
  let goodOCSPResponse = ocspResponses[0];

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function (request, response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/ocsp-response");
    response.write(goodOCSPResponse);
  });
  ocspResponder.start(8888);

  
  
  
  
  
  
  add_connection_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess);
  add_test(function () { run_next_test(); });

  add_test(function () { ocspResponder.stop(run_next_test); });

  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
  let uri = Services.io.newURI("http://localhost", null, null);
  let sslStatus = new FakeSSLStatus();
  SSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                          "max-age=10000", sslStatus, 0);
  ok(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                            "localhost", 0),
     "Domain for the OCSP AIA URI should be considered a HSTS host, otherwise" +
     " we wouldn't be testing what we think we're testing");

  run_next_test();
}
