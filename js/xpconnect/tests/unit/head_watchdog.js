







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
  setInterruptCallback(function() {
    
    
    
    if (lastWatchdogWakeup == Cu.getWatchdogTimestamp("WatchdogWakeup")) {
      return true;
    }
    do_check_true(expectInterrupt);
    setInterruptCallback(undefined);
    
    executeSoon(continuation);
    return false;
  });
  executeSoon(function() {
    busyWait(3000);
    do_check_true(!expectInterrupt);
    setInterruptCallback(undefined);
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

