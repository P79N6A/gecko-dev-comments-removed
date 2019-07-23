







































function run_test() {
  
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  var pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  var appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                   getService(Ci.nsIAppStartup);
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  var expectedQuitting;
  var called = 0;
  var observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == kPrivateBrowsingNotification &&
          aData == kExit) {
        
        ++ called;

        do_check_neq(aSubject, null);
        try {
          aSubject.QueryInterface(Ci.nsISupportsPRBool);
        } catch (ex) {
          do_throw("aSubject was not null, but wasn't an nsISupportsPRBool");
        }
        
        do_check_eq(aSubject.data, expectedQuitting);

        
        if (expectedQuitting) {
          os.removeObserver(this, kPrivateBrowsingNotification);
          prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
          do_test_finished();
        }
      }
    }
  };

  
  os.addObserver(observer, kPrivateBrowsingNotification, false);

  
  pb.privateBrowsingEnabled = true;

  
  expectedQuitting = false;
  pb.privateBrowsingEnabled = false;
  do_check_eq(called, 1);

  
  pb.privateBrowsingEnabled = true;

  
  expectedQuitting = true;
  do_test_pending();
  appStartup.quit(Ci.nsIAppStartup.eForceQuit);
}
