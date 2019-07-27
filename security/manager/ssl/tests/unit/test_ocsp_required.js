



"use strict";






let gOCSPRequestCount = 0;

function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.OCSP.require", true);
  Services.prefs.setIntPref("security.OCSP.enabled", 1);

  
  
  add_tls_server_setup("OCSPStaplingServer");

  let args = [["bad-signature", "localhostAndExampleCom", "unused"]];
  let ocspResponses = generateOCSPResponses(args, "tlsserver");
  let ocspResponseBadSignature = ocspResponses[0];

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function (request, response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/ocsp-response");
    response.write(ocspResponseBadSignature);
    gOCSPRequestCount++;
  });
  ocspResponder.start(8888);

  add_tests();

  add_test(function () { ocspResponder.stop(run_next_test); });

  run_next_test();
}

function add_tests()
{
  add_connection_test("ocsp-stapling-none.example.com",
                      SEC_ERROR_OCSP_BAD_SIGNATURE);
  add_connection_test("ocsp-stapling-none.example.com",
                      SEC_ERROR_OCSP_BAD_SIGNATURE);
  add_test(function () {
    equal(gOCSPRequestCount, 1,
          "OCSP request count should be 1 due to OCSP response caching");
    gOCSPRequestCount = 0;
    run_next_test();
  });
}
