







































function do_test() {
  
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);

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
  os.notifyObservers(null, "quit-application-granted", null);
}
