"use strict";







const TAB_STATE = {
  entries: [{url: "about:mozilla"}, {url: "about:robots"}],
  index: 1,
};

function checkTabContents(browser) {
  return ContentTask.spawn(browser, null, function* () {
    let Ci = Components.interfaces;
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory.QueryInterface(Ci.nsISHistoryInternal);
    return history && history.count == 1 && content.document.documentURI == "about:mozilla";
  });
}

add_task(function* () {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);
  yield promiseTabState(tab, TAB_STATE);

  
  let tab2 = gBrowser.addTab("about:blank");
  let browser2 = tab2.linkedBrowser;
  yield promiseBrowserLoaded(browser2);

  
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  
  let promise = promiseTabRestoring(tab2);
  ss.setTabState(tab2, JSON.stringify(TAB_STATE));
  ok(tab2.hasAttribute("pending"), "tab is pending");
  yield promise;

  
  Services.obs.notifyObservers(null, "browser:purge-session-history", "");
  ok((yield checkTabContents(browser)), "expected tab contents found");
  ok(tab2.hasAttribute("pending"), "tab is still pending");

  
  gBrowser.selectedTab = tab2;
  yield promiseTabRestored(tab2);
  ok((yield checkTabContents(browser2)), "expected tab contents found");
  ok(!tab2.hasAttribute("pending"), "tab is not pending anymore");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab);
});
