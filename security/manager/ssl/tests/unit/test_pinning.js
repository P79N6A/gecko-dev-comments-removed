























"use strict";

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);

function test_strict() {
  
  
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 2);
    run_next_test();
  });

  
  
  
  add_connection_test("unknownissuer.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE));

  
  add_connection_test("bad.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE));

  
  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  add_connection_test("sub.exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  
  
  add_connection_test("test-mode.pinning.example.com", Cr.NS_OK);
}

function test_mitm() {
  
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 1);
    run_next_test();
  });

  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);

  add_connection_test("unknownissuer.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));

  
  
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

  add_connection_test("unknownissuer.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_ISSUER));
}

function test_enforce_test_mode() {
  
  add_test(function() {
    Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 3);
    run_next_test();
  });

  add_connection_test("unknownissuer.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE));

  
  add_connection_test("bad.include-subdomains.pinning.example.com",
    getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE));

  
  add_connection_test("include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("good.include-subdomains.pinning.example.com", Cr.NS_OK);
  add_connection_test("exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  add_connection_test("sub.exclude-subdomains.pinning.example.com", Cr.NS_OK);

  
  
  
  
  add_connection_test("test-mode.pinning.example.com",
    getXPCOMStatusFromNSS(MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE));
}

function check_pinning_telemetry() {
  let service = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
  let prod_histogram = service.getHistogramById("CERT_PINNING_RESULTS")
                         .snapshot();
  let test_histogram = service.getHistogramById("CERT_PINNING_TEST_RESULTS")
                         .snapshot();
  
  
  do_check_eq(prod_histogram.counts[0], 4); 
  do_check_eq(prod_histogram.counts[1], 4); 
  do_check_eq(test_histogram.counts[0], 2); 
  do_check_eq(test_histogram.counts[1], 0); 

  let moz_prod_histogram = service.getHistogramById("CERT_PINNING_MOZ_RESULTS")
                             .snapshot();
  let moz_test_histogram =
    service.getHistogramById("CERT_PINNING_MOZ_TEST_RESULTS").snapshot();
  do_check_eq(moz_prod_histogram.counts[0], 0); 
  do_check_eq(moz_prod_histogram.counts[1], 0); 
  do_check_eq(moz_test_histogram.counts[0], 0); 
  do_check_eq(moz_test_histogram.counts[1], 0); 

  let per_host_histogram =
    service.getHistogramById("CERT_PINNING_MOZ_RESULTS_BY_HOST").snapshot();
  do_check_eq(per_host_histogram.counts[0], 0); 
  do_check_eq(per_host_histogram.counts[1], 2); 
  run_next_test();
}

function run_test() {
  add_tls_server_setup("BadCertServer");

  
  addCertFromFile(certdb, "tlsserver/other-test-ca.der", "CTu,u,u");

  test_strict();
  test_mitm();
  test_disabled();
  test_enforce_test_mode();

  add_test(function () {
    check_pinning_telemetry();
  });
  run_next_test();
}
