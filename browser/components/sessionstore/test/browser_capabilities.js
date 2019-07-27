


"use strict";






add_task(function docshell_capabilities() {
  let tab = yield createTab();
  let browser = tab.linkedBrowser;
  let docShell = browser.docShell;

  
  let flags = Object.keys(docShell).filter(k => k.startsWith("allow"));

  
  let state = JSON.parse(ss.getTabState(tab));
  ok(!("disallow" in state), "everything allowed by default");
  ok(flags.every(f => docShell[f]), "all flags set to true");

  
  docShell.allowImages = false;
  docShell.allowMetaRedirects = false;

  
  
  browser.reload();
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);

  
  let disallowedState = JSON.parse(ss.getTabState(tab));
  let disallow = new Set(disallowedState.disallow.split(","));
  ok(disallow.has("Images"), "images not allowed");
  ok(disallow.has("MetaRedirects"), "meta redirects not allowed");
  is(disallow.size, 2, "two capabilities disallowed");

  
  ss.setTabState(tab, JSON.stringify({ entries: [{url: "about:robots"}] }));
  yield promiseTabRestored(tab);

  
  TabState.flush(browser);

  
  state = JSON.parse(ss.getTabState(tab));
  ok(!("disallow" in state), "everything allowed again");
  ok(flags.every(f => docShell[f]), "all flags set to true");

  
  ss.setTabState(tab, JSON.stringify(disallowedState));
  yield promiseTabRestored(tab);

  
  ok(!docShell.allowImages, "images not allowed");
  ok(!docShell.allowMetaRedirects, "meta redirects not allowed");

  
  state = JSON.parse(ss.getTabState(tab));
  disallow = new Set(state.disallow.split(","));
  ok(disallow.has("Images"), "images not allowed anymore");
  ok(disallow.has("MetaRedirects"), "meta redirects not allowed anymore");
  is(disallow.size, 2, "two capabilities disallowed");

  
  gBrowser.removeTab(tab);
});

function createTab() {
  let tab = gBrowser.addTab("about:mozilla");
  let browser = tab.linkedBrowser;
  return promiseBrowserLoaded(browser).then(() => tab);
}
