



"use strict";

const DUMMY_PAGE = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";

function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  load(tab, DUMMY_PAGE, function() {
    gFindBar.onFindCommand();
    EventUtils.sendString("Dummy");
    gBrowser.removeTab(tab);

    try {
      gFindBar.close();
      ok(true, "findbar.close should not throw an exception");
    } catch(e) {
      ok(false, "findbar.close threw exception: " + e);
    }
    finish();
  });
}

function load(aTab, aUrl, aCallback) {
  aTab.linkedBrowser.addEventListener("load", function onload(aEvent) {
    aEvent.currentTarget.removeEventListener("load", onload, true);
    waitForFocus(aCallback, content);
  }, true);
  aTab.linkedBrowser.loadURI(aUrl);
}
