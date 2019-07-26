



"use strict";

let gFetchCount = 0;
let gGoodOCSPResponse = null;

function generateGoodOCSPResponse() {
  let args = [ ["good", "localhostAndExampleCom", "unused" ] ];
  let responses = generateOCSPResponses(args, "tlsserver");
  return responses[0];
}

function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);
  add_tls_server_setup("OCSPStaplingServer");

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function(request, response) {
    ++gFetchCount;

    do_print("gFetchCount: " + gFetchCount);

    if (gFetchCount != 2) {
      do_print("returning 500 Internal Server Error");

      response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
      let body = "Refusing to return a response";
      response.bodyOutputStream.write(body, body.length);
      return;
    }

    do_print("returning 200 OK");
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/ocsp-response");
    response.write(gGoodOCSPResponse);
  });
  ocspResponder.start(8080);

  add_tests_in_mode(true);
  add_tests_in_mode(false);

  add_test(function() { ocspResponder.stop(run_next_test); });
  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX) {
  add_test(function () {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    run_next_test();
  });

  
  

  
  
  add_connection_test("ocsp-stapling-unknown.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNKNOWN_CERT),
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 0); run_next_test(); });

  
  
  add_connection_test("ocsp-stapling-none.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_OCSP_UNKNOWN_CERT),
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 1); run_next_test(); });

  
  
  
  
  
  
  
  add_test(function() {
    let duration = 1200;
    do_print("Sleeping for " + duration + "ms");
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(run_next_test, duration, Ci.nsITimer.TYPE_ONE_SHOT);
  });
  add_test(function() {
    gGoodOCSPResponse = generateGoodOCSPResponse();
    run_next_test();
  });
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK,
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 2); run_next_test(); });

  
  
  
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK,
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 2); run_next_test(); });


  

  
  add_test(function() { clearOCSPCache(); gFetchCount = 0; run_next_test(); });

  
  
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK,
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 1); run_next_test(); });

  
  add_connection_test("ocsp-stapling-none.example.com", Cr.NS_OK,
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 1); run_next_test(); });

  
  
  add_connection_test("ocsp-stapling-revoked.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_REVOKED_CERTIFICATE),
                      clearSessionCache);
  add_test(function() { do_check_eq(gFetchCount, 1); run_next_test(); });

  

  
  add_test(function() { clearOCSPCache(); gFetchCount = 0; run_next_test(); });
}
