var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

function run_test() {
  let STSService = Cc["@mozilla.org/stsservice;1"]
                     .getService(Ci.nsIStrictTransportSecurityService);

  
  do_check_true(STSService.isStsHost("alpha.irccloud.com"));

  
  let offsetSeconds = 19 * 7 * 24 * 60 * 60;
  Services.prefs.setIntPref("test.currentTimeOffsetSeconds", offsetSeconds);

  
  do_check_false(STSService.isStsHost("alpha.irccloud.com"));

  
  Services.prefs.clearUserPref("test.currentTimeOffsetSeconds");
  do_check_true(STSService.isStsHost("alpha.irccloud.com"));
}
