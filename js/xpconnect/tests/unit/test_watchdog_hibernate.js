



function testBody() {

  setWatchdogEnabled(true);

  
  
  var now = Date.now() * 1000;
  var startHibernation = Cu.getWatchdogTimestamp("WatchdogHibernateStart");
  var stopHibernation = Cu.getWatchdogTimestamp("WatchdogHibernateStop");
  do_log_info("Pre-hibernation statistics:");
  do_log_info("now: " + now / 1000000);
  do_log_info("startHibernation: " + startHibernation / 1000000);
  do_log_info("stopHibernation: " + stopHibernation / 1000000);
  do_check_true(startHibernation < now);
  do_check_true(stopHibernation < now);

  
  
  
  
  
  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(continueTest, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
  simulateActivityCallback(false);
  yield;

  simulateActivityCallback(true);
  busyWait(1000); 
  var stateChange = Cu.getWatchdogTimestamp("RuntimeStateChange");
  startHibernation = Cu.getWatchdogTimestamp("WatchdogHibernateStart");
  stopHibernation = Cu.getWatchdogTimestamp("WatchdogHibernateStop");
  do_log_info("Post-hibernation statistics:");
  do_log_info("stateChange: " + stateChange / 1000000);
  do_log_info("startHibernation: " + startHibernation / 1000000);
  do_log_info("stopHibernation: " + stopHibernation / 1000000);
  
  
  
  
  
  const FUZZ_FACTOR = 1 * 1000 * 1000;
  do_check_true(stateChange > now + 10*1000*1000 - FUZZ_FACTOR);
  do_check_true(startHibernation > now + 2*1000*1000 - FUZZ_FACTOR);
  do_check_true(startHibernation < now + 5*1000*1000 + FUZZ_FACTOR);
  do_check_true(stopHibernation > now + 10*1000*1000 - FUZZ_FACTOR);

  do_test_finished();
  yield;
}
