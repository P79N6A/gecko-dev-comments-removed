



"use strict";




let gFetchCount = 0;
let gGoodOCSPResponse = null;
let gResponsePattern = [];
let gMessage= "";

function respondWithGoodOCSP(request, response) {
  do_print("returning 200 OK");
  response.setStatusLine(request.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "application/ocsp-response");
  response.write(gGoodOCSPResponse);
}

function respondWithError(request, response) {
  do_print("returning 500 Internal Server Error");
  response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
  let body = "Refusing to return a response";
  response.bodyOutputStream.write(body, body.length);
}

function generateGoodOCSPResponse() {
  let args = [ ["good", "localhostAndExampleCom", "unused" ] ];
  let responses = generateOCSPResponses(args, "tlsserver");
  return responses[0];
}

function add_ocsp_test(aHost, aExpectedResult, aResponses, aMessage) {
  add_connection_test(aHost, aExpectedResult,
      function() {
        clearSessionCache();
        gFetchCount = 0;
        gResponsePattern = aResponses;
        gMessage = aMessage;
      },
      function() {
        
        equal(gFetchCount, aResponses.length,
              "should have made " + aResponses.length +
              " OCSP request" + aResponses.length == 1 ? "" : "s");
      });
}

function run_test() {
  do_get_profile();
  Services.prefs.setBoolPref("security.ssl.enable_ocsp_stapling", true);
  Services.prefs.setIntPref("security.OCSP.enabled", 1);
  add_tls_server_setup("OCSPStaplingServer");

  let ocspResponder = new HttpServer();
  ocspResponder.registerPrefixHandler("/", function(request, response) {

    do_print("gFetchCount: " + gFetchCount);
    let responseFunction = gResponsePattern[gFetchCount];
    Assert.notEqual(undefined, responseFunction);

    ++gFetchCount;
    responseFunction(request, response);
  });
  ocspResponder.start(8888);

  add_tests();

  add_test(function() { ocspResponder.stop(run_next_test); });
  run_next_test();
}

function add_tests() {
  
  
  
  
  add_test(function() {
    Services.prefs.setIntPref("security.pki.cert_short_lifetime_in_days",
                              12000);
    run_next_test();
  });

  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess, [],
                "expected zero OCSP requests for a short-lived certificate");

  add_test(function() {
    Services.prefs.setIntPref("security.pki.cert_short_lifetime_in_days", 100);
    run_next_test();
  });

  
  

  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess,
                [respondWithError],
                "expected one OCSP request for a long-lived certificate");
  add_test(function() {
    Services.prefs.clearUserPref("security.pki.cert_short_lifetime_in_days");
    run_next_test();
  });
  

  
  add_test(function() { clearOCSPCache(); run_next_test(); });

  
  

  
  
  add_ocsp_test("ocsp-stapling-unknown.example.com",
                SEC_ERROR_OCSP_UNKNOWN_CERT, [],
                "Stapled Unknown response -> a fetch should not have been" +
                " attempted");

  
  
  add_ocsp_test("ocsp-stapling-none.example.com", SEC_ERROR_OCSP_UNKNOWN_CERT,
                [
                  respondWithError,
                  respondWithError,
                  respondWithError,
                  respondWithError,
                ],
                "No stapled response -> a fetch should have been attempted");

  
  
  
  
  
  
  
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
  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess,
                [respondWithGoodOCSP],
                "Cached Unknown response, no stapled response -> a fetch" +
                " should have been attempted");

  
  
  
  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess,
                [],
                "Cached Good response -> a fetch should not have been" +
                " attempted");


  

  
  add_test(function() { clearOCSPCache(); run_next_test(); });

  
  
  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess,
                [respondWithError],
                "No stapled response -> a fetch should have been attempted");

  
  add_ocsp_test("ocsp-stapling-none.example.com", PRErrorCodeSuccess, [],
                "Noted OCSP server failure -> a fetch should not have been" +
                " attempted");

  
  
  add_ocsp_test("ocsp-stapling-revoked.example.com",
                SEC_ERROR_REVOKED_CERTIFICATE, [],
                "Stapled Revoked response -> a fetch should not have been" +
                " attempted");

  

  
  add_test(function() { clearOCSPCache(); run_next_test(); });
}
