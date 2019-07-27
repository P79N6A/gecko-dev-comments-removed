















"use strict";

function run_test() {
  do_get_profile();
  add_tls_server_setup("BadCertServer");
  add_connection_test("nsCertTypeNotCritical.example.com", Cr.NS_OK);
  add_connection_test("nsCertTypeCriticalWithExtKeyUsage.example.com", Cr.NS_OK);
  add_connection_test("nsCertTypeCritical.example.com",
                      getXPCOMStatusFromNSS(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION));
  run_next_test();
}
