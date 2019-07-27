"use strict";

const URL = "http://example.com/browser_switch_remoteness_";

function countHistoryEntries(browser) {
  return ContentTask.spawn(browser, null, function* () {
    let Ci = Components.interfaces;
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory.QueryInterface(Ci.nsISHistoryInternal);
    return history && history.count;
  });
}

add_task(function* () {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);
  ok(browser.isRemoteBrowser, "browser is remote");

  
  const MAX_BACK = Services.prefs.getIntPref("browser.sessionstore.max_serialize_back");
  ok(MAX_BACK > -1, "check that the default has a value that caps data");

  
  for (let i = 0; i < MAX_BACK + 2; i++) {
    browser.loadURI(URL + i);
    yield promiseBrowserLoaded(browser);
    ok(browser.isRemoteBrowser, "browser is still remote");
  }

  
  let count = yield countHistoryEntries(browser);
  is(count, MAX_BACK + 2, "correct number of shistory entries");

  
  browser.loadURI("about:robots");
  yield promiseTabRestored(tab);
  ok(!browser.isRemoteBrowser, "browser is not remote anymore");

  
  count = yield countHistoryEntries(browser);
  is(count, MAX_BACK + 3, "correct number of shistory entries");

  
  gBrowser.removeTab(tab);
});
