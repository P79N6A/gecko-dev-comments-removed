"use strict";

const TAB_STATE = {
  entries: [{ url: "about:mozilla" }, { url: "about:robots" }],
  index: 1,
};

add_task(function* () {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  
  let promise = promiseTabRestoring(tab);
  ss.setTabState(tab, JSON.stringify(TAB_STATE));
  ok(tab.hasAttribute("pending"), "tab is pending");
  yield promise;

  
  yield TabStateFlusher.flush(browser);

  
  let tabState = TabState.collect(tab);
  is(tabState.index, TAB_STATE.index, "correct shistory index");

  
  ok(!tabState.userTypedValue, "tab didn't have a userTypedValue");

  
  gBrowser.removeTab(tab);
});
