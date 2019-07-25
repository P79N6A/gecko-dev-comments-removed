


"use strict";

function test() {
  waitForExplicitFinish();
  resetPreferences();

  registerCleanupFunction(resetPreferences);
  Services.prefs.setBoolPref("browser.search.update", false);

  open_preferences(runTest);
}

function runTest(win) {
  let doc = win.document;
  let enableSearchUpdate = doc.getElementById("enableSearchUpdate");

  win.gotoPref("paneAdvanced");

  let advancedPrefs = doc.getElementById("advancedPrefs");
  let updateTab = doc.getElementById("updateTab");
  advancedPrefs.selectedTab = updateTab;

  is_element_visible(enableSearchUpdate, "Check search update preference is visible");

  
  ok(!enableSearchUpdate.checked, "Ensure search updates are disabled");
  Services.prefs.setBoolPref("browser.search.update", true);
  ok(enableSearchUpdate.checked, "Ensure search updates are enabled");

  gBrowser.removeCurrentTab();
  win.close();
  finish();
}

function resetPreferences() {
  Services.prefs.clearUserPref("browser.search.update");
}
