






Components.utils.import("resource://gre/modules/Promise.jsm");


let XPIScope = Components.utils.import("resource://gre/modules/XPIProvider.jsm");
const DB_SCHEMA = XPIScope.DB_SCHEMA;

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

function run_test() {
  
  run_next_test();
}


function checkPending() {
  try {
    do_check_false(Services.prefs.getBoolPref("extensions.pendingOperations"));
  }
  catch (e) {
    
  }
}

function checkString(aPref, aValue) {
  try {
    do_check_eq(Services.prefs.getCharPref(aPref), aValue)
  }
  catch (e) {
    
  }
}


function check_empty_state() {
  do_check_false(gExtensionsJSON.exists());
  do_check_false(gExtensionsINI.exists());

  do_check_eq(Services.prefs.getIntPref("extensions.databaseSchema"), DB_SCHEMA);

  checkString("extensions.bootstrappedAddons", "{}");
  checkString("extensions.installCache", "[]");
  checkPending();
}








add_task(function first_run() {
  startupManager();
  check_empty_state();
  yield true;
});


function trigger_db_load() {
  let addonDefer = Promise.defer();
  AddonManager.getAddonsByTypes(['extension'], addonDefer.resolve);
  let addonList = yield addonDefer.promise;

  do_check_eq(addonList.length, 0);
  check_empty_state();

  yield true;
};
add_task(trigger_db_load);


add_task(function restart_and_recheck() {
  restartManager();
  check_empty_state();
  yield true;
});


add_task(trigger_db_load);



add_task(function upgrade_schema_version() {
  shutdownManager();
  Services.prefs.setIntPref("extensions.databaseSchema", 1);

  startupManager();
  do_check_eq(Services.prefs.getIntPref("extensions.databaseSchema"), DB_SCHEMA);
  check_empty_state();
});

