Cu.import("resource://gre/modules/Services.jsm");

function run_test() {
  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  run_test_in_child("../unit/test_bug248970_cookie.js");
}