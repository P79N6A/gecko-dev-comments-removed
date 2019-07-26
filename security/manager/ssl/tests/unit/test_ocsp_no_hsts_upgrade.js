



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
  ocspResponder.start(8080);

  add_tests_in_mode(true);
  add_tests_in_mode(false);

  add_test(function () { ocspResponder.stop(run_next_test); });

  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
  let uri = Services.io.newURI("http://localhost", null, null);
  SSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                          "max-age=10000", 0);
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "localhost", 0));

  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX) {
  add_test(function () {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    run_next_test();
  });

  
  
  
  
  
  
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK);
  add_test(function () { run_next_test(); });
}
