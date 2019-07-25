


Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gPrivateBrowsing",
                                   PRIVATEBROWSING_CONTRACT_ID,
                                   "nsIPrivateBrowsingService");

function waitForTransition(aEnabled, aCallback) {
  Services.obs.addObserver(function PBT_transition(aSubject, aTopic, aData) {
    Services.obs.removeObserver(PBT_transition, aTopic, false);
    
    
    do_execute_soon(aCallback);
  }, "private-browsing-transition-complete", false);
  gPrivateBrowsing.privateBrowsingEnabled = aEnabled;
}

function checkHistogram(aId) {
  
  
  
  let snapshot = Services.telemetry.getHistogramById(aId).snapshot();
  do_check_true(snapshot.sum > 0 || snapshot.counts[0] > 0);
}

function do_test() {
  do_test_pending();

  waitForTransition(true, function PBT_enabled() {
    waitForTransition(false, function PBT_disabled() {
      checkHistogram("PRIVATE_BROWSING_TRANSITION_ENTER_PREPARATION_MS");
      checkHistogram("PRIVATE_BROWSING_TRANSITION_ENTER_TOTAL_MS");
      checkHistogram("PRIVATE_BROWSING_TRANSITION_EXIT_PREPARATION_MS");
      checkHistogram("PRIVATE_BROWSING_TRANSITION_EXIT_TOTAL_MS");

      do_test_finished();
    });
  });
}
