


"use strict";





add_task(function setup() {
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("browser.sessionstore.restore_on_demand");
  });
});




add_task(function test_label_and_icon() {
  
  let tab = gBrowser.addTab("about:robots");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);
  let state = ss.getTabState(tab);
  gBrowser.removeTab(tab);
  browser = null;

  
  tab = gBrowser.addTab("about:blank");
  ss.setTabState(tab, state);
  yield promiseTabRestoring(tab);

  
  ok(gBrowser.getIcon(tab).startsWith("data:image/png;"), "icon is set");
  is(tab.label, "Gort! Klaatu barada nikto!", "label is set");

  
  gBrowser.removeTab(tab);
});

function promiseTabRestoring(tab) {
  let deferred = Promise.defer();

  tab.addEventListener("SSTabRestoring", function onRestoring() {
    tab.removeEventListener("SSTabRestoring", onRestoring);
    deferred.resolve();
  });

  return deferred.promise;
}
