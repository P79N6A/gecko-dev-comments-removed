


function run_test() {
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);

  
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "bugzilla.mozilla.org", 0));

  
  let offsetSeconds = 19 * 7 * 24 * 60 * 60;
  Services.prefs.setIntPref("test.currentTimeOffsetSeconds", offsetSeconds);

  
  do_check_false(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));

  
  Services.prefs.clearUserPref("test.currentTimeOffsetSeconds");
  do_check_true(SSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                       "bugzilla.mozilla.org", 0));
}
