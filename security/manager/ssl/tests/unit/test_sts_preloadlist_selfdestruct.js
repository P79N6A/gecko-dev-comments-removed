


function run_test() {
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);

  let uri = Services.io.newURI("https://bugzilla.mozilla.org", null, null);
  
  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS, uri, 0));

  
  let offsetSeconds = 19 * 7 * 24 * 60 * 60;
  Services.prefs.setIntPref("test.currentTimeOffsetSeconds", offsetSeconds);

  
  do_check_false(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS, uri, 0));

  
  Services.prefs.clearUserPref("test.currentTimeOffsetSeconds");
  do_check_true(SSService.isSecureURI(Ci.nsISiteSecurityService.HEADER_HSTS, uri, 0));
}
