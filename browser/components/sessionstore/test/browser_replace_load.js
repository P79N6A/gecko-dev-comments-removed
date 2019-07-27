"use strict";

const STATE = {
  entries: [{url: "about:robots"}, {url: "about:mozilla"}],
  selected: 2
};








add_task(function* () {
  yield testSwitchToTab("about:mozilla#fooobar", {ignoreFragment: true});
  yield testSwitchToTab("about:mozilla?foo=bar", {replaceQueryString: true});
});

let testSwitchToTab = Task.async(function* (url, options) {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  
  let promise = promiseTabRestoring(tab);
  ss.setTabState(tab, JSON.stringify(STATE));
  ok(tab.hasAttribute("pending"), "tab is pending");
  yield promise;

  
  switchToTabHavingURI(url, false, options);
  ok(!tab.hasAttribute("pending"), "tab is no longer pending");

  
  yield promiseTabRestored(tab);
  is(browser.currentURI.spec, url, "correct URL loaded");

  
  let count = yield ContentTask.spawn(browser, null, function* () {
    let Ci = Components.interfaces;
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory.QueryInterface(Ci.nsISHistoryInternal);
    return history && history.count;
  });
  is(count, 3, "three history entries");

  
  gBrowser.removeTab(tab);
});
