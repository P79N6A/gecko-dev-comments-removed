




Components.utils.import("resource://gre/modules/Services.jsm");

add_task(function* test_migrateUI() {
  Services.prefs.clearUserPref("browser.migration.version");
  Services.prefs.setBoolPref("nglayout.debug.paint_flashing", true);

  let BrowserApp = Services.wm.getMostRecentWindow("navigator:browser").BrowserApp;
  BrowserApp._migrateUI();

  
  do_check_eq(Services.prefs.getIntPref("browser.migration.version"), 1);

  
  do_check_eq(Services.prefs.prefHasUserValue("nglayout.debug.paint_flashing"), false);
});

run_next_test();
