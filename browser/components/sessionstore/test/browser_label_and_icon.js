


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

  
  yield TabStateFlusher.flush(browser);
  let state = ss.getTabState(tab);
  yield promiseRemoveTab(tab);
  browser = null;

  
  tab = gBrowser.addTab("about:blank");
  ss.setTabState(tab, state);
  yield promiseTabRestoring(tab);

  
  ok(gBrowser.getIcon(tab).startsWith("data:image/png;"), "icon is set");
  is(tab.label, "Gort! Klaatu barada nikto!", "label is set");

  
  yield promiseRemoveTab(tab);
});
