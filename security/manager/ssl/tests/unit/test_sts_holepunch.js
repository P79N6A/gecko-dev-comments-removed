



"use strict";




function run_test() {
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "chart.apis.google.com", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "CHART.APIS.GOOGLE.COM", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sub.chart.apis.google.com", 0));
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "SUB.CHART.APIS.GOOGLE.COM", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "example.apis.google.com", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "EXAMPLE.APIS.GOOGLE.COM", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "sub.example.apis.google.com", 0));
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "SUB.EXAMPLE.APIS.GOOGLE.COM", 0));
  
  let chartURI = Services.io.newURI("http://chart.apis.google.com", null, null);
  do_check_false(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       chartURI, 0));
  let otherURI = Services.io.newURI("http://other.apis.google.com", null, null);
  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS,
                                      otherURI, 0));
}
