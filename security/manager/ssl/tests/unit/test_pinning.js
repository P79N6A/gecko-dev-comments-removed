























"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);

function test_strict() {
  
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 2);
    run_next_test();
  });

  
  add_connection_test("bad.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(SEC_ERROR_APPLICATION_CALLBACK_ERROR));

  
  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  add_connection_test("sub.exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  
  
  add_connection_test("test-mode.pinning.example.com", Cr.NS_OK);
};

function test_mitm() {
  
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 1);
    run_next_test();
  });

  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  add_connection_test("bad.include-subdomains.pinning.example.com", Cr.NS_OK);

  add_connection_test("exclude-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("sub.exclude-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("test-mode.pinning.example.com", Cr.NS_OK);
};

function test_disabled() {
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 0);
    run_next_test();
  });

  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("bad.include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("exclude-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("sub.exclude-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("test-mode.pinning.example.com", Cr.NS_OK);
};

function check_pinning_telemetry() {
  let histogram = Cc["@mozilla.org/base/telemetry;1"]
                    .getService(Ci.nsITelemetry)
                    .getHistogramById("CERT_PINNING_EVALUATION_RESULTS")
                    .snapshot();
   
   do_check_eq(histogram.counts[0], 1); 
   do_check_eq(histogram.counts[1], 3); 
   run_next_test();
}

function run_test() {
  add_tls_server_setup("BadCertServer");

  
  addCertFromFile(certdb, "tlsserver/other-test-ca.der", "CTu,u,u");

  test_strict();
  test_mitm();
  test_disabled();

  add_test(function () {
    check_pinning_telemetry();
  });
  run_next_test();
}
