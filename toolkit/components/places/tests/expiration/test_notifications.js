











let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

let gObserver = {
  notifications: 0,
  observe: function(aSubject, aTopic, aData) {
    this.notifications++;
  }
};
os.addObserver(gObserver, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

function run_test() {
  
  setInterval(3600); 

  PlacesTestUtils.clearHistory();

  do_timeout(2000, check_result);
  do_test_pending();
}

function check_result() {
  os.removeObserver(gObserver, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  do_check_eq(gObserver.notifications, 1);
  do_test_finished();
}
