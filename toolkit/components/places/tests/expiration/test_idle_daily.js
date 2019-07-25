




function run_test() {
  do_test_pending();

  
  setInterval(3600); 

  Services.obs.addObserver(function observeExpiration(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observeExpiration,
                                PlacesUtils.TOPIC_EXPIRATION_FINISHED);
    do_test_finished();
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  let expire = Cc["@mozilla.org/places/expiration;1"].
               getService(Ci.nsIObserver);
  expire.observe(null, "idle-daily", null);
}
