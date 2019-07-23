







































function run_test_on_service() {
  
  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  var observer = {
    observe: function (aSubject, aTopic, aData) {
      if (aTopic == "network:offline-status-changed")
        this.events.push(aData);
    },
    events: []
  };
  os.addObserver(observer, "network:offline-status-changed", false);

  
  pb.privateBrowsingEnabled = true;
  do_check_eq(observer.events.length, 2);
  do_check_eq(observer.events[0], "offline");
  do_check_eq(observer.events[1], "online");

  
  pb.privateBrowsingEnabled = false;
  do_check_eq(observer.events.length, 4);
  do_check_eq(observer.events[2], "offline");
  do_check_eq(observer.events[3], "online");

  os.removeObserver(observer, "network:offline-status-changed", false);
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
}


function run_test() {
  run_test_on_all_services();
}
