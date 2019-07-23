






































function do_test() {
  
  var prefsService = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
  prefsService.setBoolPref("browser.privatebrowsing.autostart", true);
  do_check_true(prefsService.getBoolPref("browser.privatebrowsing.autostart"));

  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService).
           QueryInterface(Ci.nsIObserver);

  
  do_check_false(pb.autoStarted);

  
  pb.observe(null, "profile-after-change", "");

  
  do_check_true(pb.privateBrowsingEnabled);

  
  do_check_true(pb.autoStarted);

  
  pb.privateBrowsingEnabled = false;

  
  do_check_false(pb.autoStarted);

  
  pb.privateBrowsingEnabled = true;

  
  do_check_true(pb.autoStarted);
}
