



"use strict";

const PREF_INCONTENT = "browser.preferences.inContent";

let newTab = null;

add_task(function() {
  info("Check preferences button existence and functionality");

  Services.prefs.setBoolPref(PREF_INCONTENT, true);

  yield PanelUI.show();
  info("Menu panel was opened");

  let preferencesButton = document.getElementById("preferences-button");
  ok(preferencesButton, "Preferences button exists in Panel Menu");
  preferencesButton.click();

  newTab = gBrowser.selectedTab;
  yield waitForPageLoad(newTab);

  let openedPage = gBrowser.currentURI.spec;
  is(openedPage, "about:preferences", "Preferences page was opened");
});

add_task(function asyncCleanup() {
  if (gBrowser.tabs.length == 1)
    gBrowser.addTab("about:blank");

  gBrowser.removeTab(gBrowser.selectedTab);
  info("Tabs were restored");

  
  Services.prefs.clearUserPref(PREF_INCONTENT);
});

function waitForPageLoad(aTab) {
  let deferred = Promise.defer();

  let timeoutId = setTimeout(() => {
    aTab.linkedBrowser.removeEventListener("load", onTabLoad, true);
    deferred.reject("Page didn't load within " + 20000 + "ms");
  }, 20000);

  function onTabLoad(event) {
    clearTimeout(timeoutId);
    aTab.linkedBrowser.removeEventListener("load", onTabLoad, true);
    info("Tab event received: " + "load");
    deferred.resolve();
 }
  aTab.linkedBrowser.addEventListener("load", onTabLoad, true, true);
  return deferred.promise;
}
