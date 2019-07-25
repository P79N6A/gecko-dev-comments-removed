


Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "10.0");

let prefService = Services.prefs;
let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                 getService(Ci.nsIAppStartup);

const pref_last_success = "toolkit.startup.last_success";
const pref_recent_crashes = "toolkit.startup.recent_crashes";
const pref_max_resumed_crashes = "toolkit.startup.max_resumed_crashes";

function run_test() {
  resetTestEnv(0);

  test_trackStartupCrashBegin();
  test_trackStartupCrashEnd();
  test_trackStartupCrashBegin_safeMode();
  test_trackStartupCrashEnd_safeMode();
  test_maxResumed();
  resetTestEnv(0);
}


function resetTestEnv(replacedLockTime) {
  try {
    
    appStartup.trackStartupCrashBegin();
  } catch (x) { }
  prefService.setIntPref(pref_max_resumed_crashes, 2);
  prefService.clearUserPref(pref_recent_crashes);
  gAppInfo.replacedLockTime = replacedLockTime;
  prefService.clearUserPref(pref_last_success);
}

function now_seconds() {
  return ms_to_s(Date.now());
}

function ms_to_s(ms) {
  return Math.floor(ms / 1000);
}

function test_trackStartupCrashBegin() {
  let max_resumed = prefService.getIntPref(pref_max_resumed_crashes);
  do_check_false(gAppInfo.inSafeMode);

  
  let replacedLockTime = Date.now();
  resetTestEnv(replacedLockTime);
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(prefService.prefHasUserValue(pref_last_success));
  do_check_eq(replacedLockTime, gAppInfo.replacedLockTime);
  try {
    do_check_false(appStartup.trackStartupCrashBegin());
    do_throw("Should have thrown since last_success is not set");
  } catch (x) { }

  do_check_false(prefService.prefHasUserValue(pref_last_success));
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = 0;
  resetTestEnv(replacedLockTime);
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(prefService.prefHasUserValue(pref_last_success));
  do_check_eq(replacedLockTime, gAppInfo.replacedLockTime);
  try {
    do_check_false(appStartup.trackStartupCrashBegin());
    do_throw("Should have thrown since last_success is not set");
  } catch (x) { }

  do_check_false(prefService.prefHasUserValue(pref_last_success));
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = Date.now();
  resetTestEnv(replacedLockTime);
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  do_check_eq(ms_to_s(replacedLockTime), prefService.getIntPref(pref_last_success));
  do_check_false(appStartup.trackStartupCrashBegin());
  do_check_eq(ms_to_s(replacedLockTime), prefService.getIntPref(pref_last_success));
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_recent_crashes, 1);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  do_check_false(appStartup.trackStartupCrashBegin());
  do_check_eq(ms_to_s(replacedLockTime), prefService.getIntPref(pref_last_success));
  do_check_eq(1, prefService.getIntPref(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_recent_crashes, max_resumed);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  do_check_false(appStartup.trackStartupCrashBegin());
  do_check_eq(ms_to_s(replacedLockTime), prefService.getIntPref(pref_last_success));
  do_check_eq(max_resumed, prefService.getIntPref(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  do_check_true(appStartup.trackStartupCrashBegin());
  
  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  do_check_true(appStartup.automaticSafeModeNecessary);

  
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_max_resumed_crashes, -1);
  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  do_check_false(appStartup.trackStartupCrashBegin());
  
  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  
  do_check_false(appStartup.automaticSafeModeNecessary);
  do_check_eq(-1, prefService.getIntPref(pref_max_resumed_crashes));

  
  replacedLockTime = Date.now() - 365 * 24 * 60 * 60 * 1000;
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 365 * 24 * 60 * 60);
  do_check_false(appStartup.trackStartupCrashBegin());
  
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = Date.now() - 60 * 1000;
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 60 * 60); 
  do_check_false(appStartup.trackStartupCrashBegin());
  
  do_check_eq(1, prefService.getIntPref(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 60 * 60); 
  replacedLockTime = Date.now() - 60 * 1000;
  gAppInfo.replacedLockTime = replacedLockTime;
  do_check_false(appStartup.trackStartupCrashBegin());
  
  do_check_eq(2, prefService.getIntPref(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 60 * 60); 
  do_check_true(appStartup.trackStartupCrashBegin());
  
  do_check_eq(3, prefService.getIntPref(pref_recent_crashes));
  do_check_true(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = Date.now() - 365 * 24 * 60 * 60 * 1000;
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 60 * 60); 
  do_check_false(appStartup.trackStartupCrashBegin());
  
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);
}

function test_trackStartupCrashEnd() {
  
  let replacedLockTime = Date.now() - 10 * 1000; 
  resetTestEnv(replacedLockTime);
  try {
    appStartup.trackStartupCrashBegin(); 
    do_throw("Should have thrown since last_success is not set");
  } catch (x) { }
  appStartup.trackStartupCrashEnd();
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_false(prefService.prefHasUserValue(pref_last_success));

  
  replacedLockTime = Date.now() - 10 * 1000; 
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  appStartup.trackStartupCrashBegin(); 
  appStartup.trackStartupCrashEnd();
  
  
  do_check_true(prefService.getIntPref(pref_last_success) <= now_seconds());
  do_check_true(prefService.getIntPref(pref_last_success) >= now_seconds() - 4 * 60 * 60);
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));

  
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  prefService.setIntPref(pref_recent_crashes, 1);
  appStartup.trackStartupCrashBegin(); 
  appStartup.trackStartupCrashEnd();
  
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
}

function test_trackStartupCrashBegin_safeMode() {
  gAppInfo.inSafeMode = true;
  resetTestEnv(0);
  let max_resumed = prefService.getIntPref(pref_max_resumed_crashes);

  
  let replacedLockTime = Date.now() - 10 * 1000; 
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));

  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_true(prefService.prefHasUserValue(pref_last_success));
  do_check_false(appStartup.automaticSafeModeNecessary);
  do_check_false(appStartup.trackStartupCrashBegin());
  do_check_false(prefService.prefHasUserValue(pref_recent_crashes));
  do_check_true(prefService.prefHasUserValue(pref_last_success));
  do_check_false(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = Date.now() - 10 * 1000; 
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime));
  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);

  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  do_check_true(prefService.prefHasUserValue(pref_last_success));
  do_check_false(appStartup.automaticSafeModeNecessary);
  do_check_true(appStartup.trackStartupCrashBegin());
  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  do_check_true(prefService.prefHasUserValue(pref_last_success));
  do_check_true(appStartup.automaticSafeModeNecessary);

  
  replacedLockTime = Date.now() - 365 * 24 * 60 * 60 * 1000;
  resetTestEnv(replacedLockTime);
  
  let last_success = ms_to_s(replacedLockTime) - 365 * 24 * 60 * 60;
  prefService.setIntPref(pref_last_success, last_success);
  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);
  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  do_check_true(prefService.prefHasUserValue(pref_last_success));
  do_check_true(appStartup.automaticSafeModeNecessary);
  do_check_true(appStartup.trackStartupCrashBegin());
  do_check_eq(max_resumed + 1, prefService.getIntPref(pref_recent_crashes));
  do_check_eq(last_success, prefService.getIntPref(pref_last_success));
  do_check_true(appStartup.automaticSafeModeNecessary);
}

function test_trackStartupCrashEnd_safeMode() {
  gAppInfo.inSafeMode = true;
  let replacedLockTime = Date.now();
  resetTestEnv(replacedLockTime);
  let max_resumed = prefService.getIntPref(pref_max_resumed_crashes);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 24 * 60 * 60);

  
  prefService.setIntPref(pref_recent_crashes, 1);
  appStartup.trackStartupCrashBegin(); 
  appStartup.trackStartupCrashEnd();
  do_check_eq(1, prefService.getIntPref(pref_recent_crashes));

  
  
  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);
  appStartup.trackStartupCrashBegin(); 
  appStartup.trackStartupCrashEnd();
  do_check_eq(max_resumed, prefService.getIntPref(pref_recent_crashes));
}

function test_maxResumed() {
  resetTestEnv(0);
  gAppInfo.inSafeMode = false;
  let max_resumed = prefService.getIntPref(pref_max_resumed_crashes);
  let replacedLockTime = Date.now();
  resetTestEnv(replacedLockTime);
  prefService.setIntPref(pref_max_resumed_crashes, -1);

  prefService.setIntPref(pref_recent_crashes, max_resumed + 1);
  prefService.setIntPref(pref_last_success, ms_to_s(replacedLockTime) - 24 * 60 * 60);
  appStartup.trackStartupCrashBegin();
  
  do_check_eq(max_resumed + 2, prefService.getIntPref(pref_recent_crashes));
  do_check_false(appStartup.automaticSafeModeNecessary);
}