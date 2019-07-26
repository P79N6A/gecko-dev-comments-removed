



"use strict";





let gOCSPRequestCount = 0;

function add_ocsp_test(aHost, aExpectedResult) {
  add_connection_test(aHost, aExpectedResult,
    function() {
      clearOCSPCache();
      clearSessionCache();
    });
}

function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function(request, response) {
    gOCSPRequestCount++;
    response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
    let body = "Refusing to return a response";
    response.bodyOutputStream.write(body, body.length);
  });
  ocspResponder.start(8080);

  add_tls_server_setup("OCSPStaplingServer");

  add_ocsp_test("ocsp-stapling-with-intermediate.example.com", Cr.NS_OK);

  add_test(function () { ocspResponder.stop(run_next_test); });
  add_test(function() {
    do_check_eq(gOCSPRequestCount, 0);
    run_next_test();
  });
  run_next_test();
}
