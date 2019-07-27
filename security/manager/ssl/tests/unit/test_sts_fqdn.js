



"use strict";

function run_test() {
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com.", 0));
  
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com..", 0));

  let uri = Services.io.newURI("https://example.com", null, null);
  let sslStatus = new FakeSSLStatus();
  SSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                          "max-age=1000;includeSubdomains", sslStatus, 0);
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "example.com", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "example.com.", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "example.com..", 0));

  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS,
                                      uri, 0));
  uri = Services.io.newURI("https://example.com.", null, null);
  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS,
                                      uri, 0));
  uri = Services.io.newURI("https://example.com..", null, null);
  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS,
                                      uri, 0));

  SSService.removeState(Ci.nsISiteSecurityService.HEADER_HSTS, uri, 0);
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com.", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "example.com..", 0));
}
