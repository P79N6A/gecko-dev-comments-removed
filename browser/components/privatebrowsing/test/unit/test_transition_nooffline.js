







































function run_test_on_service() {
  
  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

  var observer = {
    observe: function (aSubject, aTopic, aData) {
      if (aTopic == "network:offline-status-changed")
        this.events.push(aData);
    },
    events: []
  };
  os.addObserver(observer, "network:offline-status-changed", false);

  
  pb.privateBrowsingEnabled = true;
  do_check_eq(observer.events.length, 0);

  
  pb.privateBrowsingEnabled = false;
  do_check_eq(observer.events.length, 0);

  os.removeObserver(observer, "network:offline-status-changed", false);
}


function run_test() {
  run_test_on_all_services();
}
