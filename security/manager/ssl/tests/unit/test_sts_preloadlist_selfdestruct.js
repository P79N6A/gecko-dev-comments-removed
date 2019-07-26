


function run_test() {
  let STSService = Cc["@mozilla.org/stsservice;1"]
                     .getService(Ci.nsIStrictTransportSecurityService);

  
  do_check_true(STSService.isStsHost("bugzilla.mozilla.org", 0));

  
  let offsetSeconds = 19 * 7 * 24 * 60 * 60;
  Services.prefs.setIntPref("test.currentTimeOffsetSeconds", offsetSeconds);

  
  do_check_false(STSService.isStsHost("bugzilla.mozilla.org", 0));

  
  Services.prefs.clearUserPref("test.currentTimeOffsetSeconds");
  do_check_true(STSService.isStsHost("bugzilla.mozilla.org", 0));
}
