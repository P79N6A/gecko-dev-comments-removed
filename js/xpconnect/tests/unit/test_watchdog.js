







const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var gPrefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

function setWatchdogEnabled(enabled) {
  gPrefs.setBoolPref("dom.use_watchdog", enabled);
}

function isWatchdogEnabled() {
  return gPrefs.getBoolPref("dom.use_watchdog");
}





function busyWait(ms) {
  var start = new Date();
  while ((new Date()) - start < ms) {}
}

function do_log_info(aMessage)
{
  print("TEST-INFO | " + _TEST_FILE + " | " + aMessage);
}




function executeSoon(fn) {
  var tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  tm.mainThread.dispatch({run: fn}, Ci.nsIThread.DISPATCH_NORMAL);
}










function checkWatchdog(expectInterrupt, continuation) {
  var lastWatchdogWakeup = Cu.getWatchdogTimestamp("WatchdogWakeup");
  setOperationCallback(function() {
    
    
    
    if (lastWatchdogWakeup == Cu.getWatchdogTimestamp("WatchdogWakeup")) {
      return true;
    }
    do_check_true(expectInterrupt);
    setOperationCallback(undefined);
    
    executeSoon(continuation);
    return false;
  });
  executeSoon(function() {
    busyWait(3000);
    do_check_true(!expectInterrupt);
    setOperationCallback(undefined);
    continuation();
  });
}

var gGenerator;
function continueTest() {
  gGenerator.next();
}

function run_test() {

  
  do_test_pending();

  
  gGenerator = testBody();
  gGenerator.next();
}

function testBody() {

  
  
  checkWatchdog(isWatchdogEnabled(), continueTest);
  yield;

  
  var was = isWatchdogEnabled();
  setWatchdogEnabled(!isWatchdogEnabled());
  do_check_true(was != isWatchdogEnabled());
  checkWatchdog(isWatchdogEnabled(), continueTest);
  yield;

  
  setWatchdogEnabled(true);
  do_check_true(isWatchdogEnabled());
  checkWatchdog(true, continueTest);
  yield;

  
  
  

  
  
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
